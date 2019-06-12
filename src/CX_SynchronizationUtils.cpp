#include "CX_SynchronizationUtils.h"

#include "CX_Draw.h"

namespace CX {

namespace Instances {
	CX::Sync::DomainSynchronizer DomainSync;
}

namespace Sync {

	bool areTimesWithinTolerance(const CX_Millis& a, const CX_Millis& b, const CX_Millis& tolerance) {
		cxTick_t absDif = abs(a.nanos() - b.nanos());
		return absDif < tolerance.nanos();
	}

	////////////////////
	// TimePrediction //
	////////////////////

	TimePrediction::TimePrediction(void) :
		pred(TimeError),
		predictionIntervalHalfWidth(PredictionIntervalWarning),
		usable(false)
	{}

	CX_Millis TimePrediction::lowerBound(void) const {
		return pred - predictionIntervalHalfWidth;
	}
	CX_Millis TimePrediction::prediction(void) const {
		return pred;
	}
	CX_Millis TimePrediction::upperBound(void) const {
		return pred + predictionIntervalHalfWidth;
	}

	// trivial to calculate, if you think of it. this function is the reminder.
	// if equal to 1, the predictions are essentially useless: you could easily be off by an entire swap period.
	// maybe 0.2 swap periods is good enough?
	double TimePrediction::getPredIntWidthWithRespectToSwapPeriod(CX_Millis period) const {
		return 2 * predictionIntervalHalfWidth / period;
	}
	
	////////////////////////
	// SwapUnitPrediction //
	////////////////////////

	SwapUnitPrediction::SwapUnitPrediction(void) :
		fp{0, 0, 0},
		usable(false)
	{}

	SwapUnit SwapUnitPrediction::lowerBound(Util::Rounding rounding) const {
		return (SwapUnit)CX::Util::round(fp.lower, 0, rounding);
	}

	SwapUnit SwapUnitPrediction::prediction(Util::Rounding rounding) const {
		return (SwapUnit)CX::Util::round(fp.pred, 0, rounding);
	}

	SwapUnit SwapUnitPrediction::upperBound(Util::Rounding rounding) const {
		return (SwapUnit)CX::Util::round(fp.upper, 0, rounding);
	}


	///////////////////////
	// DataContainer //
	///////////////////////

	DataContainer::DataContainer(void) :
		_timeStoreNextSwapUnit(0)
	{}

	void DataContainer::setup(const Configuration& config) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);

		_stopListeningToSources();

		_config = config;

		_timeStoreNextSwapUnit = 0;
	}

	DataContainer::Configuration DataContainer::getConfiguration(void) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		return _config;
	}

	/*! Receive data from another DataContianer.
	*/
	void DataContainer::receiveFrom(DataContainer* container) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);

		_stopListeningToSources();

		_containerSourceHelper.setup<DataContainer>(&container->newDataEvent, this, &DataContainer::_containerSourceCallback);
	}

	void DataContainer::_containerSourceCallback(const NewData& data) {
		if (!data.empty()) {
			storeSwap(data.newest());
		}
	}

	/*! Receive data from an event that sends swap data (i.e. `const SwapData&`). 
	This might receive from CX_Display or CX_SoundStream.
	*/
	void DataContainer::receiveFrom(ofEvent<const SwapData&>* eventSource) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);

		_stopListeningToSources();

		_eventSourceHelper.setup<DataContainer>(eventSource, this, &DataContainer::_eventSourceCallback);
	}
	
	void DataContainer::_eventSourceCallback(const SwapData& data) {
		storeSwap(data);
	}
	
	/*! Receive data from an event that sends time data (i.e. `const CX_Millis&`).
	This might recieve from CX_Clock. 
	*/
	void DataContainer::receiveFrom(ofEvent<const CX_Millis&>* eventSource) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);

		_stopListeningToSources();

		_eventSourceMillisHelper.setup<DataContainer>(eventSource, this, &DataContainer::_eventSourceMillisCallback);
	}
	
	void DataContainer::_eventSourceMillisCallback(const CX_Millis& time) {
		this->storeSwap(time);
	}
	

	void DataContainer::_stopListeningToSources(void) {
		_eventSourceHelper.stopListening();
		_eventSourceHelper.stopListening();
		_containerSourceHelper.stopListening();
	}



	void DataContainer::storeSwap(CX_Millis time) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);

		// Higher latency values values mean more latency, so subtract latency
		time -= _config.latency; 

		_data.push_back(SwapData(time, _timeStoreNextSwapUnit));
		// Size a time was stored, the next swap unit has to be inferred, so increment by units per swap
		_timeStoreNextSwapUnit += _config.unitsPerSwap;

		// remove excess data
		while (_data.size() > _config.sampleSize) {
			_data.pop_front();
		}

		this->newDataEvent.notify(NewData(_data));
	}

	void DataContainer::storeSwap(SwapData data) {

		std::lock_guard<std::recursive_mutex> lock(_mutex);

		// Higher latency values values mean more latency, so subtract latency
		data.time -= _config.latency;

		_data.push_back(data);

		// remove excess data
		while (_data.size() > _config.sampleSize) {
			_data.pop_front();
		}

		this->newDataEvent.notify(NewData(_data));

	}

	size_t DataContainer::size(void) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		return _data.size();
	}

	bool DataContainer::full(void) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		return _data.size() == _config.sampleSize;
	}

	void DataContainer::clear(bool keepLastSample, bool resetSwapUnit) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);

		SwapData last;
		if (!_data.empty()) {
			last = _data.back();
		} else {
			keepLastSample = false;
		}

		_data.clear();

		if (resetSwapUnit) {
			last.unit = 0; // reset last to 0
			_timeStoreNextSwapUnit = _config.unitsPerSwap; // set next to 0 plus unitsPerSwap
		}

		if (keepLastSample) {
			_data.push_back(std::move(last));
		}

		this->newDataEvent.notify(NewData(_data));
	}

	void DataContainer::setLatency(CX_Millis latency) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);

		// Update stored times based on the new latency
		CX_Millis latencyUpdate = _config.latency - latency;
		for (SwapData& d : _data) {
			d.time += latencyUpdate;
		}
		// Or should you just clear the data?

		_config.latency = latency;

		this->newDataEvent.notify(NewData(_data));
	}

	CX_Millis DataContainer::getLatency(void) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		return _config.latency;
	}

	void DataContainer::setSampleSize(size_t size) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		_config.sampleSize = size;
	}

	/*! Set the minimum sample size. If the current sample size is less than `minSize`, 
	the sample will be set to `minSize`. Otherwise, nothing is changed. */
	void DataContainer::setMinimumSampleSize(size_t minSize) {
		if (minSize > getSampleSize()) {
			setSampleSize(minSize);
		}
	}

	size_t DataContainer::getSampleSize(void) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		return _config.sampleSize;
	}

	void DataContainer::setNominalSwapPeriod(CX_Millis period) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		_config.nominalSwapPeriod = period;
	}

	CX_Millis DataContainer::getNominalSwapPeriod(void) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		return _config.nominalSwapPeriod;
	}

	SwapUnit DataContainer::getUnitsPerSwap(void) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		return _config.unitsPerSwap;
	}

	SwapUnit DataContainer::getNextSwapUnit(void) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);

		if (_data.empty()) {
			// TODO: what to do?????
		}

		return getLastSwapData().unit + _config.unitsPerSwap;
	}

	DataContainer::LockedDataPointer DataContainer::getLockedDataPointer(void) {
		return LockedDataPointer(&_data, _mutex);
	}

	// yeah, I'm just gonna go ahead and say that copying the data is not that costly. it's three 64 bit ints per one of maybe 100 samples.
	std::deque<SwapData> DataContainer::copyData(void) {
		_mutex.lock();
		std::deque<SwapData> copy(_data);
		_mutex.unlock();
		return std::move(copy);
	}

	CX_Millis DataContainer::getLastSwapTime(void) {
		return getLastSwapData().time;
	}

	SwapUnit DataContainer::getLastSwapUnit(void) {
		return getLastSwapData().unit;
	}

	SwapData DataContainer::getLastSwapData(void) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);

		if (_data.empty()) {
			return SwapData();
		}
		return _data.back();
	}

	////////////////////////////
	// DataContainer::NewData //
	////////////////////////////


	DataContainer::NewData::NewData(const std::deque<SwapData>& d) :
		data(d)
	{}

	bool DataContainer::NewData::empty(void) const {
		return data.empty();
	}

	const SwapData& DataContainer::NewData::newest(void) const {
		return data.back();
	}

	const SwapData& DataContainer::NewData::oldest(void) const {
		return data.front();
	}


	///////////////////////////////////////
	// DataContainer::PolledSwapListener //
	///////////////////////////////////////

	DataContainer::PolledSwapListener::PolledSwapListener(DataContainer* cont) :
		_container(cont),
		_hasSwapped(false)
	{
		_lastDataPoint = _container->getLastSwapData();
	}

	// \return What it says on the tin. An immediate call to this function after calling setup() will return false.
	bool DataContainer::PolledSwapListener::hasSwappedSinceLastCheck(void) {
		getNewestData();
		bool rval = _hasSwapped;
		_hasSwapped = false;
		return rval;
	}

	SwapData DataContainer::PolledSwapListener::getNewestData(void) {
		SwapData thisDataPoint = _container->getLastSwapData();

		if (thisDataPoint.unit != _lastDataPoint.unit) {
			_hasSwapped = true;
			_lastDataPoint = thisDataPoint;
		}

		return thisDataPoint;
	}

	bool DataContainer::PolledSwapListener::waitForSwap(CX_Millis timeout, bool reset) {

		if (reset) {
			hasSwappedSinceLastCheck();
		}

		CX_Millis endTime = CX::Instances::Clock.now() + timeout;
		do {
			if (hasSwappedSinceLastCheck()) {
				return true;
			}

			//Instances::Input.pollEvents();

			std::this_thread::yield();

		} while (CX::Instances::Clock.now() > endTime);
		return false;
	}

	//////////////////////////
	// StabilityVerifier //
	//////////////////////////

	StabilityVerifier::StabilityVerifier(void) :
		_lastStatus(Status::Uninitialized)
	{}

	bool StabilityVerifier::setup(const Configuration& config) {
		if (config.dataContainer == nullptr) {
			return false;
		}

		std::lock_guard<std::recursive_mutex> lock(_mutex);

		_config = config;

		if (_config.sampleSize < 2) {
			Instances::Log.warning("StabilityVerifier") << "setup(): config.sampleSize must be at least 2, but it was not. sampleSize was set to 2.";
			_config.sampleSize = 2;
		}

		_calcConfig.nominalSwapPeriod = _config.dataContainer->getNominalSwapPeriod();
		_calcConfig.stoppageInterval = _calcConfig.nominalSwapPeriod * _config.stoppagePeriodMultiplier;
		_calcConfig.intervalTolerance = _calcConfig.nominalSwapPeriod * _config.swapPeriodTolerance;

		_lastStatus = Status::InsufficientData;
		_newDataAvailable = false;

		_config.dataContainer->setMinimumSampleSize(_config.sampleSize);

		_newDataEventHelper.setup<StabilityVerifier>(&_config.dataContainer->newDataEvent, this, &StabilityVerifier::_newDataEventHandler);

		return true;
	}


	StabilityVerifier::Status StabilityVerifier::_getStatus(const std::deque<SwapData>& data) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);

		if (data.size() < 1) {
			return Status::InsufficientData;
		}

		const SwapData& lastDataPoint = data.back();

		// only need one data point to determine stoppage
		CX_Millis timeFromLastDataPoint = Instances::Clock.now() - lastDataPoint.time;
		if (timeFromLastDataPoint > _calcConfig.stoppageInterval) {
			return Status::Stopped;
		}

		if (data.size() < _config.sampleSize) {
			return Status::InsufficientData;
		}

		// number of periods is 1 less than sample size
		size_t pastEnd = data.size() - 1;
		size_t startIndex = data.size() - _config.sampleSize;

		for (size_t i = startIndex; i < pastEnd; i++) {

			CX_Millis interval = data[i + 1].time - data[i].time;

			if (!areTimesWithinTolerance(interval, _calcConfig.nominalSwapPeriod, _calcConfig.intervalTolerance)) {
				return Status::SwappingUnstably;
			}
		}

		return Status::SwappingStably;
	}

	StabilityVerifier::Status StabilityVerifier::getStatus(void) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);

		if (!_newDataAvailable) {
			return _lastStatus;
		}

		DataContainer::LockedDataPointer ldp = _config.dataContainer->getLockedDataPointer();

		return _getStatus(*ldp);
	}

	std::string StabilityVerifier::getStatusString(Status status) {
		switch (status) {
		case Status::InsufficientData:
			return "InsufficientData";
		case Status::Uninitialized:
			return "Uninitialized";
		case Status::Stopped:
			return "Stopped";
		case Status::SwappingStably:
			return "SwappingStably";
		case Status::SwappingUnstably:
			return "SwappingUnstably";
		}
		return "";
	}

	bool StabilityVerifier::isSwappingStably(void) {
		return getStatus() == Status::SwappingStably;

	}
	bool StabilityVerifier::waitForStableSwapping(CX_Millis timeout) {
		CX_Millis endTime = Instances::Clock.now() + timeout;

		do {
			if (isSwappingStably()) {
				return true;
			}
		} while (Instances::Clock.now() < endTime);

		return false;
	}

	/*
	double StabilityVerifier::getProportionOfStatuses(Status stat) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);

		if (_statusCounter.find(stat) == _statusCounter.end()) {
			_statusCounter[stat] = 0;
		}

		double total = 0.000001; // avoid divide by 0
		for (auto& pr : _statusCounter) {
			total += pr.second;
		}

		return _statusCounter[stat] / total;
	}
	*/

	void StabilityVerifier::_newDataEventHandler(const DataContainer::NewData& data) {
		if (data.empty()) {
			//this->clear(); // ?
			return;
		}

		std::lock_guard<std::recursive_mutex> lock(_mutex);

		if (_config.autoUpdate) {
			Status currentStatus = _getStatus(data.data);

			if (currentStatus != _lastStatus) {
				ofNotifyEvent(statusChangeEvent, currentStatus);
				_lastStatus = currentStatus;
			}
		} else {
			_newDataAvailable = true;
		}

	}


	/////////////////////
	// LinearModel //
	/////////////////////


	bool LinearModel::setup(const Configuration& config) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);

		if (config.dataContainer == nullptr) {
			_newDataEventHelper.stopListening();
			return false;
		}

		_config = config;

		_newDataAvailable = false;

		_config.dataContainer->setMinimumSampleSize(_config.sampleSize);

		_newDataEventHelper.setup<LinearModel>(&_config.dataContainer->newDataEvent, this, &LinearModel::_newDataListener);

		return true;
	}

	/*
	bool LinearModel::setDataSource(DataContainer* dataSource, bool autoUpdate) {

		std::lock_guard<std::recursive_mutex> lock(_mutex);

		_newDataAvailable = false;

		_config.dataSource = dataSource;
		_config.autoUpdate = autoUpdate;

		if (dataSource) {
			_newDataEventHelper.setup<LinearModel>(&_config.dataSource->newDataEvent, this, &LinearModel::_newDataListener);
		} else {
			_newDataEventHelper.stopListening();
		}

	}
	*/

	LinearModel::Configuration LinearModel::getConfiguration(void) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		return _config;
	}

	LinearModel::LockedFittedModel LinearModel::getLockedFittedModel(void) {
		
		fitModel(); // only fits if new data available

		return LockedFittedModel(&_fm, _mutex);
	}

	LinearModel::FittedModel LinearModel::copyFittedModel(void) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		fitModel();
		return _fm;
	}

	void LinearModel::_newDataListener(const DataContainer::NewData& nd) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);

		_newDataAvailable = true;

		if (_config.autoUpdate) {
			this->fitModel(&nd.data);
		}

	}

	bool LinearModel::fitModel(const std::deque<SwapData>* data) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		_fm = _fitModel(*data);
		_newDataAvailable = false;
		return _fm.fittedSuccessfully;
	}

	bool LinearModel::fitModel(DataContainer* store) {
		if (!store) {
			return false;
		}
		auto lockedData = store->getLockedDataPointer();
		return this->fitModel(lockedData.get());
	}

	bool LinearModel::fitModel(void) {
		if (_newDataAvailable && !_config.autoUpdate) {
			return this->fitModel(_config.dataContainer);
		}
		return true;
	}

	LinearModel::FittedModel LinearModel::_fitModel(const std::deque<SwapData>& data) {

		_mutex.lock();
		size_t sampleSize = _config.sampleSize;
		_mutex.unlock();

		FittedModel fm;
		fm.fittedSuccessfully = false;

		if (data.size() < sampleSize) {
			Instances::Log.error("Sync::LinearModel") << "fitModel(): Insufficient data. Need " << sampleSize << " samples and have " << data.size() << " samples.";
			return fm;
		}

		size_t startIndex = data.size() - sampleSize;

		fm.N = data.size();

		fm.xBar = 0;
		fm.yBar = 0;
		for (size_t i = startIndex; i < data.size(); i++) {
			fm.xBar += data[i].unit;
			fm.yBar += data[i].time.millis();
		}
		fm.xBar /= fm.N;
		fm.yBar /= fm.N;


		fm._numSum = 0;
		fm._denSum = 0;

		for (size_t i = startIndex; i < data.size(); i++) {

			double xDif = data[i].unit - fm.xBar;
			double yDif = data[i].time.millis() - fm.yBar;

			fm._numSum += xDif * yDif;
			fm._denSum += xDif * xDif;
		}

		fm.slope = CX_Millis(fm._numSum / fm._denSum);
		fm.intercept = CX_Millis(fm.yBar) - fm.slope * fm.xBar;

		// mark as fitted before calculating MSE and residuals
		fm.fittedSuccessfully = true;

		// calculate the residuals and MSE
		fm.residuals.resize(fm.N);
		fm.MSE = 0;
		for (size_t i = startIndex; i < data.size(); i++) {

			CX_Millis predY = fm.calculateTime(data[i].unit);
			fm.residuals[i] = data[i].time - predY;

			double dresid = fm.residuals[i].millis();
			fm.MSE += dresid * dresid;
		}
		fm.MSE /= (fm.N - 2);

		return fm;
	}

	//////////////////////////////////
	// LinearModel::FittedModel //
	//////////////////////////////////

	TimePrediction LinearModel::FittedModel::predictTimeFP(double unit) const {

		TimePrediction tp;

		if (!_fittedSuccessfully(true)) {
			return tp;
		}

		tp.pred = this->calculateTimeFP(unit);


		double qt = LinearModel::FittedModel::_getQT((int)this->N - 2);
		double xDif = unit - xBar;
		double rhRad = 1.0 + (1.0 / this->N) + (xDif * xDif) / _denSum;

		tp.predictionIntervalHalfWidth = qt * sqrt(this->MSE) * sqrt(rhRad);
		tp.usable = true;

		return tp;

	}

	SwapUnitPrediction LinearModel::FittedModel::predictSwapUnit(const TimePrediction& tp) const {

		SwapUnitPrediction sup;

		if (!_fittedSuccessfully(true)) {
			return sup;
		}

		sup.fp.pred = this->calculateSwapUnitFP(tp.pred);

		sup.fp.lower = this->calculateSwapUnitFP(tp.lowerBound());
		sup.fp.upper = this->calculateSwapUnitFP(tp.upperBound());

		sup.usable = true;

		return sup;
	}


	TimePrediction LinearModel::FittedModel::predictTime(SwapUnit unit) const {
		return predictTimeFP((double)unit);
	}

	SwapUnitPrediction LinearModel::FittedModel::predictSwapUnit(CX_Millis time) const {
		return predictSwapUnit(predictTimeFP(calculateSwapUnitFP(time)));
	}

	CX_Millis LinearModel::FittedModel::calculateTime(SwapUnit unit) const {
		return calculateTimeFP((double)unit);
	}

	SwapUnit LinearModel::FittedModel::calculateSwapUnit(CX_Millis time) const {
		return SwapUnit(calculateSwapUnitFP(time)); // rounding?
	}

	CX_Millis LinearModel::FittedModel::calculateTimeFP(double swapUnit) const {
		if (!_fittedSuccessfully(true)) {
			return 0;
		}
		return slope * swapUnit + intercept;
	}

	double LinearModel::FittedModel::calculateSwapUnitFP(CX_Millis time) const {
		if (!_fittedSuccessfully(true)) {
			return 0;
		}
		return (time - intercept) / slope;
	}



	unsigned int LinearModel::FittedModel::degreesOfFreedom(void) const {
		return N - 2; // estimate 2 param
	}

	double LinearModel::FittedModel::_getQT(int df) {
		// 95% interval
		static std::vector<double> qtLUT = { 12.7062047362, 4.3026527297, 3.1824463053, 2.7764451052, 2.5705818356, 2.4469118511, 2.3646242516, 2.3060041352, 2.2621571628, 2.2281388520, 2.2009851601, 2.1788128297, 2.1603686565, 2.1447866879, 2.1314495456, 2.1199052992, 2.1098155778, 2.1009220402, 2.0930240544, 2.0859634473, 2.0796138447, 2.0738730679, 2.0686576104, 2.0638985616, 2.0595385528, 2.0555294386, 2.0518305165, 2.0484071418, 2.0452296421, 2.0422724563 };

		if (df <= 0) {
			CX::Instances::Log.error("LinearModel") << "Invaid degrees of freedom for t-distribution quantile look-up-table.";
			return 0;
		}

		if (df > (int)qtLUT.size()) {
			df = (int)qtLUT.size();
		}

		return qtLUT[df - 1];
	}

	bool LinearModel::FittedModel::_fittedSuccessfully(bool warn) const {
		if (warn && !fittedSuccessfully) {
			Instances::Log.warning("LinearModel::FittedModel") << "Attempt to access model results when model was not fitted successfully. See LinearModel::FittedModel::fittedSuccessfully.";
		}

		return fittedSuccessfully;
	}

	////////////////////////
	// DomainSynchronizer //
	////////////////////////


	void DomainSynchronizer::addDataClient(std::string clientName, DataClient* client) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		auto it = _clients.find(clientName);
		if (it != _clients.end()) {
			Instances::Log.warning("DomainSynchronizer") << "addDataClient(): Synchronizer \"" << clientName << "\" replaced.";
		}
		_clients[clientName] = client;
	}

	void DomainSynchronizer::removeDataClient(std::string clientName) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		auto it = _clients.find(clientName);
		if (it != _clients.end()) {
			_clients.erase(it);
			Instances::Log.notice("DomainSynchronizer") << "removeDataClient(): Synchronizer \"" << clientName << "\" removed.";
		} else {
			Instances::Log.warning("DomainSynchronizer") << "removeDataClient(): Synchronizer \"" << clientName << "\" not found.";
		}
	}

	void DomainSynchronizer::clearDataClients(void) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		_clients.clear();
	}

	bool DomainSynchronizer::allReady(void) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);

		bool allReady = true;
		for (auto& it : _clients) {
			allReady = allReady && it.second->allReady();
		}
		return allReady;
	}

	bool DomainSynchronizer::waitUntilAllReady(CX_Millis timeout) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);

		CX_Millis endTime = Instances::Clock.now() + timeout;
		do {

			if (this->allReady()) {
				return true;
			}

		} while (Instances::Clock.now() < endTime);

		return false;
	}

	std::string DomainSynchronizer::getStatusString(void) {

		std::ostringstream oss;

		oss << "DomainSynchronizer status: " << std::endl;

		for (auto& it : _clients) {

			oss << std::endl;

			oss << "DataClient " << it.first << ":" << std::endl;
			oss << "All ready: " << it.second->allReady() << std::endl;
			std::string statusString = StabilityVerifier::getStatusString(it.second->verifier.getStatus());
			oss << "Verifier status: " << statusString << std::endl;

		}

		return oss.str();
	}

	SyncPoint DomainSynchronizer::getSyncPoint(CX_Millis time) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);

		SyncPoint sp;
		sp.time.usable = true;
		sp.time.pred = time; // ugh
		sp.time.predictionIntervalHalfWidth = 0; // no uncertainty about the input time

		// TODO: It seems like you will need to lock the mutexes of all of the data clients at the same time, right?
		// No, because the predictions should be fairly stable WRT new data points (otherwise the predictions are not very useful).
		// You don't need to lock the mutexes.

		for (auto& it : _clients) {
			SyncPoint::ClientData& spd = sp.clientData[it.first];
			DataClient* sync = it.second;

			spd.allReady = sync->allReady();
			spd.status = sync->verifier.getStatus();

			if (!spd.allReady) {
				continue;
			}
			
			LinearModel::LockedFittedModel lfm = sync->lm.getLockedFittedModel();
			spd.pred = lfm->predictSwapUnit(time);

		}

		return sp;
	}

	SyncPoint DomainSynchronizer::getSyncPoint(std::string clientName, SwapUnit unit) {

		std::lock_guard<std::recursive_mutex> lock(_mutex);

		SyncPoint sp;

		{
			// Predict time from this data client
			DataClient* thisSync = _getDataClient(clientName);
			if (!thisSync) {
				return sp;
			}

			{
				LinearModel::LockedFittedModel lfm = thisSync->lm.getLockedFittedModel();
				sp.time = lfm->predictTime(unit);
			}

			SyncPoint::ClientData& spd = sp.clientData[clientName];

			// no uncertainty about the input unit. note that this is lossy!
			spd.pred.usable = true;
			spd.pred.fp.lower = unit;
			spd.pred.fp.pred = unit;
			spd.pred.fp.upper = unit;

			spd.allReady = thisSync->allReady();
			spd.status = thisSync->verifier.getStatus();
		}


		for (auto& it : _clients) {
			if (it.first == clientName) {
				continue;
			}

			SyncPoint::ClientData& spd = sp.clientData[it.first];
			DataClient* sync = it.second;

			spd.allReady = sync->allReady();
			spd.status = sync->verifier.getStatus();

			if (!spd.allReady) {
				continue;
			}

			LinearModel::LockedFittedModel lfm = sync->lm.getLockedFittedModel();
			spd.pred = lfm->predictSwapUnit(sp.time);

		}

		return sp;
	}

	DomainSynchronizer::DCLP DomainSynchronizer::getDCLP(std::string clientName) {
		_mutex.lock();
		DataClient* sync = _getDataClient(clientName);
		return DCLP(sync, _mutex, std::adopt_lock);
	}

	DataClient* DomainSynchronizer::_getDataClient(std::string clientName) {
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		auto it = _clients.find(clientName);
		if (it == _clients.end()) {
			return nullptr;
		}
		return it->second;
	}



	//////////////////
	// DataClient //
	//////////////////

	bool DataClient::setup(const Configuration& config) {

		if (!config.dataContainer) {
			return false;
		}

		// set up a new data event from the data store so that lm and verifier get the new data (at least if auto updating)
		_config = config;

		size_t sampleSize = std::ceil(_config.dataCollectionDuration / _config.dataContainer->getNominalSwapPeriod());
		if (sampleSize < 3) {
			CX::Instances::Log.warning("Sync::DataClient") << 
				"The swap sample size was less than 3 for data collection of " << _config.dataCollectionDuration.seconds() << 
				" seconds. The sample size must be at least 3 and was set to 3.";
			sampleSize = 3;
		}

		// Make sure the data container is storing enough data.
		_config.dataContainer->setMinimumSampleSize(sampleSize);


		//
		LinearModel::Configuration lmConfig;

		lmConfig.dataContainer = _config.dataContainer;
		lmConfig.autoUpdate = _config.autoUpdate;
		lmConfig.sampleSize = sampleSize;

		lm.setup(lmConfig);

		//
		StabilityVerifier::Configuration slvConfig;

		slvConfig.dataContainer = _config.dataContainer;
		slvConfig.autoUpdate = _config.autoUpdate;

		slvConfig.swapPeriodTolerance = _config.swapPeriodTolerance;
		slvConfig.sampleSize = sampleSize;

		slvConfig.stoppagePeriodMultiplier = _config.stoppagePeriodMultiplier;

		verifier.setup(slvConfig);

		return true;

	}


	bool DataClient::allReady(void) {

		if (verifier.getStatus() != StabilityVerifier::Status::SwappingStably) {
			return false;
		}

		LinearModel::LockedFittedModel lfm = lm.getLockedFittedModel();
		if (!lfm->fittedSuccessfully) {
			return false;
		}

		return true;
	}

	bool DataClient::waitUntilAllReady(CX_Millis timeout) {

		CX_Millis endTime = Instances::Clock.now() + timeout;

		do {
			if (allReady()) {
				return true;
			}
		} while (Instances::Clock.now() > endTime);

		return false;
	}

	SwapUnitPrediction DataClient::predictSwapUnitAtTime(CX_Millis time) {
		if (!this->allReady()) {
			return Sync::SwapUnitPrediction();
		}

		Sync::LinearModel::LockedFittedModel lfm = this->lm.getLockedFittedModel();
		return lfm->predictSwapUnit(time);
	}

	TimePrediction DataClient::predictSwapTime(SwapUnit swapUnit) {
		if (!this->allReady()) {
			return Sync::TimePrediction();
		}

		Sync::LinearModel::LockedFittedModel lfm = this->lm.getLockedFittedModel();
		return lfm->predictTime(swapUnit);
	}

	TimePrediction DataClient::predictSwapTimeFP(double unit) {
		if (!this->allReady()) {
			return Sync::TimePrediction();
		}

		Sync::LinearModel::LockedFittedModel lfm = this->lm.getLockedFittedModel();
		return lfm->predictTimeFP(unit);
	}

	TimePrediction DataClient::predictLastSwapTime(void) {
		_mutex.lock();
		SwapData lastData = _config.dataContainer->getLastSwapData();
		_mutex.unlock();

		Sync::TimePrediction rval = predictSwapTime(lastData.unit);

		if (!rval.usable) {
			rval.pred = lastData.time;
			rval.predictionIntervalHalfWidth = PredictionIntervalWarning;
			rval.usable = true;
		}

		return rval;
	}

	TimePrediction DataClient::predictNextSwapTime(void) {

		_mutex.lock();
		SwapUnit nextSwapUnit = _config.dataContainer->getNextSwapUnit();
		_mutex.unlock();

		Sync::TimePrediction rval = predictSwapTime(nextSwapUnit);

		if (!rval.usable) {
			_mutex.lock();
			rval.pred = _config.dataContainer->getLastSwapData().time + _config.dataContainer->getNominalSwapPeriod();
			rval.predictionIntervalHalfWidth = PredictionIntervalWarning;
			rval.usable = true;
			_mutex.unlock();
		}

		return rval;

	}

	TimePrediction DataClient::predictTimeToNextSwap(void) {

		Sync::TimePrediction rval = predictNextSwapTime();
		rval.pred -= Instances::Clock.now();

		return rval;
	}


} // namespace Sync
} // namespace CX