#pragma once

#include <map>
#include <mutex>
#include <deque>

#include "CX_Time_t.h"
#include "CX_Utilities.h"
#include "CX_ThreadUtils.h"
#include "CX_UnitConversion.h"
#include "CX_Events.h"

/* \namespace CX::Sync

This namespace contains utilities related to synchronizing stimulus events, specifically audio and visual stimuli, but the method generalizes to other stimuli.

It is conceptually important to analyze the components of a computer into:
1. The CPU, which knows what time it is.
2. Stimulus presenting hardware/software stacks. The video stack includes the graphics API software (OpenGL for CX), additional operating system software, drivers (software at OS level), the video card (hardware), additional software running on the video card, and the monitor hardware/firmware.

CPU -> Massive data bus -> Chipset (on motherboard) -> PCIe bus -> video card (including the software running on the video card) -> cable to monitor (VGA/DVI/HDMI/DP/etc.) -> monitor (including software running on the monitor)



Both sound and video are presented by computers using a buffer swapping approach. 
For video, a single frame is a whole buffer, presented at a rate of 30 - 240 frames per second (with a rapidly increasing upper limit circa 2020).
For sound, a buffer typically contains around one thousand audio samples that are presented at a rate of 48 to 192 thousand samples per second.

The monitor displaying video stimuli knows when it has swapped in a new frame, but the CPU does not.
Similarly, the sound card may know when it is playing a particular sample, but again, the CPU does not.
Software does not yet fully connect all of the components of a computer.
There are not standard ways for the CPU to learn about the monitor's frame presentations.

The best information that the CPU has about when stimuli are presented is when the stimulus buffers swap.
For example, with sound, typically 2 buffers are used alternately. While the sound samples in one buffer are being
presented by the sound card (i.e. being converted from digital to analog, specifically analog voltages that drive physical speakers),
the other buffer is available to have the next set of sound samples written to it by the CPU.

Video hardware/software works analagously to sound, with an active frame that is displayed on screen (in OpenGL terms, the "front buffer"),
and an inactive buffer that is available to be written to ("back buffer").

I can't make strong statements about the exact nature of buffer swapping across various hardwares and softwares,
but buffer swapping is ubiquitous and works similarly across platforms.

There are time constraints on the buffer swapping processes:
TC1. The back buffer must have all of the necessary stimulus data written to it by the CPU before it is swapped in (becomes the front buffer).
TC2. The front buffer must have all of its information read out of it by the stimulus hardware/software before it is swapped out.
TC3. The amount of time it takes for the CPU to fill a buffer

Timeline:
Assume that the front buffer (FB) contains valid stimulus information and is currently being presented (this tends to be true).

The CPU works on filling the back buffer (BB) with stimulus information. 
Whenever the CPU is done (the scene is fully drawn or the next batch of sound samples are loaded), it notifies the stimulus hardware/software stack.

BB is filled by CPU.
BB is released by CPU, given to simulus hardware.
Whenever the hardware is done with the old FB, it swaps the buffers: The old FB becomes the new BB and while 
FB is filled by CPU.
FB is given by CPU to stimulus hardware.




To capture a timestamp related to stimulus presentation, it makes most sense to use the time at which the stimulus hardware makes an empty buffer (the old front buffer) available to the CPU.
The time at which the back buffer has been filled and is given to stimulus hardware is less appropriate because the amount of time it takes to fill the back buffer depends on what is being put into the back buffer 
(for sound, at least the way CX is designed, each buffer filling should take the same amount of time, but visual stimuli will take varying amounts of time).



*/

namespace CX {
namespace Sync {

class StabilityVerifier;
class DataClient;


typedef uint64_t SwapUnit;
typedef int64_t SwapUnitDif;

const CX_Millis TimeError = CX_Millis::max();
const CX_Millis PredictionIntervalWarning = CX_Minutes(1);

const SwapUnit SwapUnitError = std::numeric_limits<SwapUnit>::max(); // I don't expect swap events to ever be able count up to the max value


bool areTimesWithinTolerance(const CX_Millis& a, const CX_Millis& b, const CX_Millis& tolerance);

/*! Contains data about a stimulus buffer swap. */
struct SwapData {

	SwapData(void) :
		time(TimeError),
		unit(SwapUnitError)
	{}

	SwapData(CX_Millis t, SwapUnit u) :
		time(t),
		unit(u)
	{}

	CX_Millis time; //!< The CPU time at which the swap happened.
	SwapUnit unit; //!< A non-decreasing integral value. For video, this counts presented frames. For audio, this counts presented sample frames, of which many sample frames are presented per buffer swap. This means that `unit` does not count buffer swaps!
};

/*! Stores information about a time prediction and has helper functions that return the 
predicted value and lower and upper bounds on the prediction interval. */
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


// EventTracker and EventProcessor?
class DataContainer {
public:

	typedef CX::Util::LockedPointer<const std::deque<SwapData>, std::recursive_mutex> LockedDataPointer;
	using LockedDataReference = CX::Util::LockedReference<const std::deque<SwapData>, std::recursive_mutex>;

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

	//DataContainer(void);

	void setup(const Configuration& config);
	Configuration getConfiguration(void);

	// Store swaps directly
	void storeSwap(CX_Millis time);
	void storeSwap(SwapData swapData);

	// Or receive swaps from somewhere else
	void receiveFrom(DataContainer* container);
	void receiveFrom(std::shared_ptr<DataContainer> container);
	void receiveFrom(ofEvent<const SwapData&>* eventSource);
	void receiveFrom(ofEvent<const CX_Millis&>* eventSource);


	size_t size(void); // stored? storedCount?
	bool full(void); // atCapacity?
	void clear(bool keepLastSample = true, bool resetSwapUnit = true);


	void setLatency(CX_Millis latency);
	CX_Millis getLatency(void); // getConfiguration   TODO remove these getConfiguration

	void setSampleSize(size_t size);
	void setMinimumSampleSize(size_t minSize);
	size_t getSampleSize(void); // getConfiguration

	
	void setNominalSwapPeriod(CX_Millis period);
	CX_Millis getNominalSwapPeriod(void); // getConfiguration

	// Units per swap cannot be set other than by setup because it requires a clear
	SwapUnit getUnitsPerSwap(void); // getConfiguration
	void setUnitsPerSwap(SwapUnit units);

	SwapData getLastSwapData(void);
	CX_Millis getLastSwapTime(void);
	SwapUnit getLastSwapUnit(void);

	SwapUnit getNextSwapUnit(void);

	
	LockedDataPointer getLockedDataPointer(void);
	LockedDataReference getLockedDataReference(void);
	std::vector<SwapData> copyData(void);


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
	SwapData _lastData;

	//SwapUnit _timeStoreNextSwapUnit; // TODO: This seems funky

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

/*! Implements a basic bivariate linear model.

*/
class LinearModel {
public:

	/*! Contains information about a model fitted with `LinearModel`.

	The special functionality provided by `FittedModel` is help with predicting swap times/units.
	
	*/
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
		DataContainer* dataContainer; // can be nullptr
		
		size_t sampleSize; // will use the most recent sampleSize samples

		bool autoUpdate;
	};

	bool setup(const Configuration& config);
	Configuration getConfiguration(void);

	bool fitModel(void); // uses DataContainer from config
	bool fitModel(const std::deque<SwapData>& data);
	bool fitModel(DataContainer* data);
	bool fitModel(std::shared_ptr<DataContainer> data);

	bool newFitAvailable(void);

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

	FittedModel _fitModel(const std::deque<SwapData>& data, size_t sampleSize);
	bool _newFitAvailable;
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
	const Configuration& getConfiguration(void) const;


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
	void addDataClient(std::string clientName, std::shared_ptr<DataClient> client);
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

	std::recursive_mutex _mutex;
	std::map<std::string, std::shared_ptr<DataClient>> _clients;

	std::shared_ptr<DataClient> _getDataClient(std::string name);
};



} // namespace Sync

namespace Instances {
	extern CX::Sync::DomainSynchronizer DomainSync;
}

} // namespace CX