#pragma once

#include <map>
#include <mutex>
#include <deque>

#include "CX_Time_t.h"
#include "CX_Utilities.h"
#include "CX_ThreadUtils.h"
#include "CX_UnitConversion.h"

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

	//float alpha; // set alpha = 0.05 (or whatever was used), even if the user can't request an alpha?
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
			latency(0),
			unitsPerSwap(1)
			//eventSource(nullptr),
			//containerSource(nullptr)
		{}

		CX_Millis nominalSwapPeriod;
		SwapUnit unitsPerSwap;

		size_t sampleSize;

		CX_Millis latency; // + values: more latency, so subtract latency

		// choose one. if not nullptr, will be listened to for data
		//ofEvent<const SwapData&>* eventSource; 
		//DataContainer* containerSource;
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
	bool full(void); // what???
	void clear(bool keepLastSample = true, bool resetSwapUnit = true);


	void setLatency(CX_Millis latency);
	CX_Millis getLatency(void); // getConfiguration

	void setSampleSize(size_t size);
	void setMinimumSampleSize(size_t minSize);
	size_t getSampleSize(void); // getConfiguration

	// cannot be set, other than by setup
	void setNominalSwapPeriod(CX_Millis period);
	CX_Millis getNominalSwapPeriod(void);

	//void setUnitsPerSwap(SwapUnit unitsPerSwap);
	SwapUnit getUnitsPerSwap(void);

	
	LockedDataPointer getLockedDataPointer(void);
	std::deque<SwapData> copyData(void);

	SwapData getNewestDataPoint(void);
	SwapUnit getSwapUnitForNextSwap(void);

	struct NewData {

		NewData(const std::deque<SwapData>& d) :
			data(d)
		{}

		const std::deque<SwapData>& data;

		bool empty(void) const {
			return data.empty();
		}

		const SwapData& newest(void) const {
			return data.back();
		}

		const SwapData& oldest(void) const {
			return data.front();
		}
	};

	ofEvent<const NewData&> newDataEvent;

	

	struct PolledSwapListener {

		PolledSwapListener(DataContainer* cont) {
			_container = cont;
			_lastDataPoint = _container->getNewestDataPoint();
		}

		// \return What it says on the tin. An immediate call to this function after calling setup() will return false.
		bool hasSwappedSinceLastCheck(void) {
			SwapData thisDataPoint = _container->getNewestDataPoint();

			if (thisDataPoint.unit != _lastDataPoint.unit) {
				_lastDataPoint = thisDataPoint;
				return true;
			}
			return false;
		}

		SwapData getNewestData(void) {
			SwapData thisDataPoint = _container->getNewestDataPoint();
			if (thisDataPoint.unit != _lastDataPoint.unit) {
				_lastDataPoint = thisDataPoint;
			}
			return _lastDataPoint;
		}

		bool waitForSwap(CX_Millis timeout, bool reset = true) {

			if (reset) {
				hasSwappedSinceLastCheck();
			}

			CX_Millis endTime = Instances::Clock.now() + timeout;
			do {
				if (hasSwappedSinceLastCheck()) {
					return true;
				}

				//Instances::Input.pollEvents();

				std::this_thread::yield();

			} while (Instances::Clock.now() > endTime);
			return false;
		}

	private:
		DataContainer* _container;
		SwapData _lastDataPoint;
	};



	std::unique_ptr<PolledSwapListener> getPolledSwapListener(void) {
		return std::make_unique<PolledSwapListener>(this);
	}

	// not as useful as PolledSwapListener
	/*
	struct CallbackSwapListener {

		CallbackSwapListener(DataContainer* container) {
			setup(container);
		}

		void setup(DataContainer* container) {
			_helper.setup<CallbackSwapListener>(&container->newDataEvent, this, &CallbackSwapListener::_callbackHandler);
		}

		void setCallback(std::function<void(const NewData&)> callback) {
			_mutex.lock();
			_callback = callback;
			_mutex.unlock();
		}

	private:

		std::mutex _mutex;

		std::function<void(const NewData&)> _callback;

		Util::ofEventHelper<const NewData&> _helper;

		void _callbackHandler(const NewData& data) {
			_mutex.lock();
			_callback(data);
			_mutex.unlock();
		}

	};

	std::unique_ptr<CallbackSwapListener> getCallbackSwapListener(void) {
		return std::make_unique<CallbackSwapListener>(this);
	}
	*/



private:

	std::recursive_mutex _mutex;

	Configuration _config;

	std::deque<SwapData> _data;

	SwapUnit _timePushNextSwapUnit;

	std::shared_ptr<PolledSwapListener> _polledSwapListener;

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
			swapPeriodTolerance(0.5),
			stoppagePeriodMultiplier(3)
		{}

		DataContainer* dataContainer;
		bool autoUpdate; // now that updating is costlier, it makes sense to have auto update as an option.

		size_t sampleSize; // `sampleSize` is in swap events, so results in sampleSize - 1 swap periods. Thus, sampleSize must be >= 2.
		//unsigned int requiredPeriods;

		//CX_Millis nominalSwapPeriod;

		//CX_Millis swapPeriodBias; // optional bias (can be estimated with LinearModel)
		double swapPeriodTolerance; // proportion of nominalSwapPeriod

		double stoppagePeriodMultiplier; // multiple of nominalSwapPeriod
	};

	// is there some way to check for missed swaps? have a window for the next two swap times and if the first is missed but second on time, note missed swap

	StabilityVerifier(void);

	bool setup(const Configuration& config);

	Status getStatus(void);
	static std::string getStatusString(Status status);

	bool isSwappingStably(void); // shouldn't this take an int of some kind?
	bool waitForStableSwapping(CX_Millis timeout);

	ofEvent<Status> statusChangeEvent; // any time the status changes

private:

	std::recursive_mutex _mutex;

	Configuration _config;
	struct {
		CX_Millis nominalSwapPeriod;
		CX_Millis stoppageInterval;
		CX_Millis intervalTolerance;
		//CX_Millis missedSwapPeriodLength;
	} _calcConfig;

	CX::Util::ofEventHelper<const DataContainer::NewData&> _newDataEventHelper;
	void _newDataEventHandler(const DataContainer::NewData& data);
	bool _newDataAvailable;

	Status _lastStatus;

	Status _getStatus(const std::deque<SwapData>& data);
};


struct SyncPointClientData {
	bool allReady;
	SwapUnitPrediction pred;
	StabilityVerifier::Status status;
};

struct SyncPoint {
	TimePrediction time;
	std::map<std::string, SyncPointClientData> clientData;

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

		TimePrediction _predictTime(double swapUnit) const;


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

	//bool setDataSource(DataContainer* dataSource, bool autoUpdate = true);

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

	// why? Data Client Locked Pointer
	typedef CX::Util::LockedPointer<DataClient, std::recursive_mutex> DCLP;
	DCLP getDCLP(std::string clientName);

private:
	// no mutex for these guys, except when changing the contents of this container, not when accessing the pointers
	std::recursive_mutex _mutex; // i don't think so. this doesn't work like this. this is a general mutex on _syncs
	std::map<std::string, DataClient*> _clients;

	DataClient* _getDataClient(std::string name);
};



class DataClient {
public:

	LinearModel lm;
	StabilityVerifier verifier;

	struct Configuration {
		DataContainer* dataContainer;
		bool autoUpdate; // only updates LM, verifier always auto updates

		double swapPeriodTolerance; // for verifier

		CX_Millis dataCollectionDuration; // used for sample size for lm and verifier
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

class DataVisualizer {
public:

	struct Configuration {
		Sync::DataContainer* data;

		Sync::LinearModel::Configuration lmConfig;
	};

	void setup(const Configuration& config) {

		_newDataHelper.setup<DataVisualizer>(&config.data->newDataEvent, this, &DataVisualizer::_newDataCallback);

		_lm.setup(config.lmConfig);
	}

	bool startRecording(void) {
		_recording = true;
	}

	void stopRecording(void) {

	}

	void startPlaying(double speedMultiplier) {

		_speedMultiplier = speedMultiplier;
		_currentSample = -1;

		_playing = true;

		//_lmData.clear();
		_lmData.assign(_recordedSamples.begin(), _recordedSamples.begin() + _config.lmConfig.sampleSize);
		_lm.fitModel(&_lmData);
	}

	void playLive(CX_Millis autoRecordDuration) {

		//_autoRecordSamples = 

	}

	void updatePlayback(void) {
		_drawDisplay(Instances::Clock.now(), _drawConfig, _recordedSamples);
	}



private:
	std::recursive_mutex _mutex;

	Configuration _config;

	bool _recording;
	bool _playing;
	bool _playingLive;
	double _speedMultiplier;
	CX_Millis _playbackStartTime;
	size_t _autoRecordSamples;

	Sync::LinearModel _lm;
	std::deque<SwapData> _lmData;
	//Sync::DataContainer _lmDataContainer;

	std::deque<SwapData> _recordedSamples;
	int _currentSample;

	SwapData* getNextSample(void) {
		int next = _currentSample + 1;
		if (next < 0 || next >= _recordedSamples.size()) {
			return nullptr;
		}
		return &_recordedSamples[next];
	}


	Util::ofEventHelper<const Sync::DataContainer::NewData&> _newDataHelper;
	void _newDataCallback(const Sync::DataContainer::NewData& nd);

	struct DrawConfig {
		ofRectangle viewport;
		ofPoint viewOffset;


		float lineWidth;
		ofColor slopeLineCol;
		ofColor predLineCol;
		ofColor gridLineCol;

		ofColor dataPointCol;


		SwapUnit baseDataWidth;
		CX_Millis baseDataHeight;
		float zoom;

	} _drawConfig;

	void _drawDisplay(CX_Millis t, DrawConfig dc, const std::deque<SwapData>& fullData);


};


} // namespace Sync

namespace Instances {
	extern CX::Sync::DomainSynchronizer DomainSync;
}

} // namespace CX