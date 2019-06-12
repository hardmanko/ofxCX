#pragma once

#include <map>
#include <mutex>
#include <deque>

#include "CX_Time_t.h"
#include "CX_Utilities.h"
#include "CX_ThreadUtils.h"
#include "CX_UnitConversion.h"
#include "CX_Events.h"

namespace CX {
namespace Sync {

class StabilityVerifier;
class DataClient;


typedef uint64_t SwapUnit;
typedef int64_t SwapUnitDif;

const CX_Millis TimeError = CX_Millis::max();
const CX_Millis PredictionIntervalWarning = CX_Minutes(1);

const SwapUnit SwapUnitError = std::numeric_limits<SwapUnit>::max(); // I don't expect swap events to ever be able count up to the max value

struct SwapData {

	SwapData(void) :
		time(TimeError),
		unit(SwapUnitError)
	{}

	SwapData(CX_Millis t, SwapUnit u) :
		time(t),
		unit(u)
	{}

	CX_Millis time;
	SwapUnit unit;
};


bool areTimesWithinTolerance(const CX_Millis& a, const CX_Millis& b, const CX_Millis& tolerance);


struct TimePrediction {

	TimePrediction(void);

	CX_Millis lowerBound(void) const; // min ?
	CX_Millis prediction(void) const;
	CX_Millis upperBound(void) const; // max ?

	double getPredIntWidthWithRespectToSwapPeriod(CX_Millis period) const;

	bool usable;
	CX_Millis pred;
	CX_Millis predictionIntervalHalfWidth;

	//float alpha; // Show users what alpha was used even if the user can't request an alpha?
};

struct SwapUnitPrediction {

	SwapUnitPrediction(void);

	SwapUnit lowerBound(Util::Rounding rounding = Util::Rounding::ToNearest) const;
	SwapUnit prediction(Util::Rounding rounding = Util::Rounding::ToNearest) const;
	SwapUnit upperBound(Util::Rounding rounding = Util::Rounding::ToNearest) const;

	bool usable;
	struct {
		double lower;
		double pred;
		double upper;
	} fp;
};



class DataContainer {
public:

	typedef CX::Util::LockedPointer<const std::deque<SwapData>, std::recursive_mutex> LockedDataPointer;

	struct Configuration {

		Configuration(void) :
			unitsPerSwap(1),
			sampleSize(0)
		{}

		CX_Millis nominalSwapPeriod;
		SwapUnit unitsPerSwap;
		size_t sampleSize;

		CX_Millis latency;
	};

	DataContainer(void);

	void setup(const Configuration& config);
	Configuration getConfiguration(void);

	void receiveFrom(DataContainer* container);
	void receiveFrom(ofEvent<const SwapData&>* eventSource);
	void receiveFrom(ofEvent<const CX_Millis&>* eventSource);

	void storeSwap(CX_Millis time);
	void storeSwap(SwapData swapData);

	size_t size(void);
	bool full(void);
	void clear(bool keepLastSample = true, bool resetSwapUnit = true);


	void setLatency(CX_Millis latency);
	CX_Millis getLatency(void); // getConfiguration   TODO remove these getConfiguration

	void setSampleSize(size_t size);
	void setMinimumSampleSize(size_t minSize);
	size_t getSampleSize(void); // getConfiguration

	
	void setNominalSwapPeriod(CX_Millis period);
	CX_Millis getNominalSwapPeriod(void); // getConfiguration

	// Units per swap cannot be set other than by setup. Why?
	SwapUnit getUnitsPerSwap(void); // getConfiguration

	
	LockedDataPointer getLockedDataPointer(void);
	std::deque<SwapData> copyData(void); // return vector<SwapData>?

	SwapData getLastSwapData(void);
	CX_Millis getLastSwapTime(void);
	SwapUnit getLastSwapUnit(void);

	SwapUnit getNextSwapUnit(void);


	struct NewData {
		NewData(const std::deque<SwapData>& d);

		const std::deque<SwapData>& data;

		bool empty(void) const;
		const SwapData& newest(void) const;
		const SwapData& oldest(void) const;
	};

	ofEvent<const NewData&> newDataEvent;

	
	struct PolledSwapListener {
		PolledSwapListener(DataContainer* cont);

		SwapData getNewestData(void);
		bool hasSwappedSinceLastCheck(void);
		bool waitForSwap(CX_Millis timeout, bool reset = true);

	private:
		DataContainer* _container;
		bool _hasSwapped;
		SwapData _lastDataPoint;
	};

	std::unique_ptr<PolledSwapListener> getPolledSwapListener(void) {
		return std::make_unique<PolledSwapListener>(this);
	}


private:

	std::recursive_mutex _mutex;

	Configuration _config;

	std::deque<SwapData> _data;

	SwapUnit _timeStoreNextSwapUnit; // TODO: This seems funky

	void _stopListeningToSources(void);

	Util::ofEventHelper<const SwapData&> _eventSourceHelper;
	void _eventSourceCallback(const SwapData& data);

	Util::ofEventHelper<const CX_Millis&> _eventSourceMillisHelper;
	void _eventSourceMillisCallback(const CX_Millis& time);

	Util::ofEventHelper<const NewData&> _containerSourceHelper;
	void _containerSourceCallback(const NewData& data);

};



class StabilityVerifier {
public:

	enum class Status : int {
		Uninitialized = -3,
		Stopped = -2, // last event received longer than nominalSwapPeriod * stoppagePeriodMultiplier ago
		SwappingUnstably = -1, // at least one interval problem in last sampleSize swaps
		InsufficientData = 0, // less than sampleSize swaps
		SwappingStably = 1 // no interval problems in last sampleSize swaps (or sampleSize - 1 intervals)
	};

	struct Configuration {

		Configuration(void) :
			dataContainer(nullptr),
			sampleSize(0),
			swapPeriodTolerance(0.5),
			stoppagePeriodMultiplier(3),
			autoUpdate(false)
		{}

		DataContainer* dataContainer;
		size_t sampleSize; // `sampleSize` is in swap events, so results in sampleSize - 1 swap periods. Thus, sampleSize must be >= 2.

		double swapPeriodTolerance; // proportion of nominalSwapPeriod
		double stoppagePeriodMultiplier; // multiple of nominalSwapPeriod

		bool autoUpdate; // now that updating is costlier, it makes sense to have auto update as an option.
	};

	StabilityVerifier(void);

	bool setup(const Configuration& config);

	Status getStatus(void);
	static std::string getStatusString(Status status);

	bool isSwappingStably(void);
	bool waitForStableSwapping(CX_Millis timeout);

	ofEvent<Status> statusChangeEvent; //!< Event triggers every time the status changes.

private:

	std::recursive_mutex _mutex;

	Configuration _config;
	struct {
		CX_Millis nominalSwapPeriod;
		CX_Millis stoppageInterval;
		CX_Millis intervalTolerance;
	} _calcConfig;

	CX::Util::ofEventHelper<const DataContainer::NewData&> _newDataEventHelper;
	void _newDataEventHandler(const DataContainer::NewData& data);
	bool _newDataAvailable;

	Status _lastStatus;

	Status _getStatus(const std::deque<SwapData>& data);
};




struct SyncPoint {

	struct ClientData {

		ClientData(void) :
			allReady(false)
		{}

		bool allReady;
		SwapUnitPrediction pred;
		StabilityVerifier::Status status;
	};

	TimePrediction time;
	std::map<std::string, ClientData> clientData;

	bool valid(void) const {
		if (clientData.size() == 0) {
			return false;
		}
		for (const auto& cl : clientData) {
			if (!cl.second.allReady) {
				return false;
			}
		}
		return true;
	}
};

class LinearModel {
public:

	struct FittedModel {

		bool fittedSuccessfully;

		unsigned int N; // sample size

		CX_Millis slope;
		CX_Millis intercept;

		std::vector<CX_Millis> residuals;
		double MSE;

		double yBar;
		double xBar;

		// predict functions give uncertainties
		TimePrediction predictTime(SwapUnit unit) const;
		
		SwapUnitPrediction predictSwapUnit(CX_Millis time) const;
		SwapUnitPrediction predictSwapUnit(const TimePrediction& tp) const;

		// calculate functions give no uncertainties
		CX_Millis calculateTime(SwapUnit unit) const;
		SwapUnit calculateSwapUnit(CX_Millis time) const;

		// floating point swap unit
		TimePrediction predictTimeFP(double unit) const;
		CX_Millis calculateTimeFP(double swapUnit) const;
		double calculateSwapUnitFP(CX_Millis time) const;
		
		unsigned int degreesOfFreedom(void) const;

	private:

		static double _getQT(int df); // maybe this should go into Private or just be a loose function in Sync
		bool _fittedSuccessfully(bool warn = true) const;

		friend class LinearModel;

		double _numSum; // sum_i (x_i - xBar) * (y_i - yBar)
		double _denSum; // sum_i (x_i - xBar)^2
	};

	typedef CX::Util::LockedPointer<const FittedModel, std::recursive_mutex> LockedFittedModel;

	struct Configuration {
		DataContainer* dataContainer;
		bool autoUpdate;

		size_t sampleSize; // will use the most recent sampleSize samples
	};

	bool setup(const Configuration& config);
	Configuration getConfiguration(void);

	bool fitModel(const std::deque<SwapData>* data);
	bool fitModel(DataContainer* store);
	bool fitModel(void); // uses data store from config

	LockedFittedModel getLockedFittedModel(void);
	FittedModel copyFittedModel(void);

private:

	std::recursive_mutex _mutex;

	Configuration _config;

	FittedModel _fm;

	// for auto update, listeners to swap events...
	bool _newDataAvailable;
	void _newDataListener(const DataContainer::NewData& dp);
	CX::Util::ofEventHelper<const DataContainer::NewData&> _newDataEventHelper;

	FittedModel _fitModel(const std::deque<SwapData>& data);
};




class DataClient {
public:

	LinearModel lm;
	StabilityVerifier verifier;

	struct Configuration {

		Configuration(void) :
			dataContainer(nullptr),
			dataCollectionDuration(CX_Seconds(1)),
			autoUpdate(false),
			swapPeriodTolerance(0.5),
			stoppagePeriodMultiplier(3)
		{}

		// for both LM and verifier
		DataContainer* dataContainer;
		CX_Millis dataCollectionDuration;
		bool autoUpdate; 

		// for verifier
		double swapPeriodTolerance;
		double stoppagePeriodMultiplier;
		
	};

	bool setup(const Configuration& config);

	bool allReady(void);
	bool waitUntilAllReady(CX_Millis timeout);

	// safer than using lm directly
	TimePrediction predictSwapTime(SwapUnit unit);
	TimePrediction predictLastSwapTime(void);
	TimePrediction predictNextSwapTime(void);
	TimePrediction predictTimeToNextSwap(void);

	TimePrediction predictSwapTimeFP(double unit);
	
	SwapUnitPrediction predictSwapUnitAtTime(CX_Millis time);

private:
	std::recursive_mutex _mutex;
	Configuration _config;

	Util::ofEventHelper<const std::deque<SwapData>&> _newDataEventHelper;
};


// synchronizes across multiple time domains
class DomainSynchronizer {
public:

	void addDataClient(std::string clientName, DataClient* client);
	void removeDataClient(std::string clientName);
	void clearDataClients(void);

	bool allReady(void);
	bool waitUntilAllReady(CX_Millis timeout);
	std::string getStatusString(void);

	SyncPoint getSyncPoint(CX_Millis time);
	SyncPoint getSyncPoint(std::string clientName, SwapUnit unit);

	// Data Client Locked Pointer
	typedef CX::Util::LockedPointer<DataClient, std::recursive_mutex> DCLP;
	DCLP getDCLP(std::string clientName);

private:
	// no mutex for these guys, except when changing the contents of this container, not when accessing the pointers
	std::recursive_mutex _mutex; // i don't think so. this doesn't work like this. this is a general mutex on _syncs
	std::map<std::string, DataClient*> _clients;

	DataClient* _getDataClient(std::string name);
};



} // namespace Sync

namespace Instances {
	extern CX::Sync::DomainSynchronizer DomainSync;
}

} // namespace CX