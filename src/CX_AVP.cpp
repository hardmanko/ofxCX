#include "CX_AVP.h"

#include "CX_EntryPoint.h"

namespace CX {


bool CX_AVP::setup(const Configuration& config) {

	if (config.display == nullptr || config.soundStream == nullptr) {
		return false;
	}

	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_config = config;
	_displayThread = _config.display->getDisplayThread();

	_soundPlayer.setup(_config.soundStream);

	//_configureSyncer(_config.sync);
	_configureDomainSync();

	_dispSwapListener = _config.display->swapData.getPolledSwapListener();

	slides.setup(config.display);
	//_slideHelper.setup(&slides, _config.display);

	bool modeSetupSuccess = false;
	switch (_config.mode) {
	case Mode::RM_SM_Helped:
		modeSetupSuccess = RMSMH_setup();
		break;
	case Mode::RM_ST_Helped:
		modeSetupSuccess = RMSTH_setup();
		break;
	case Mode::RT_ST_Helped:
		modeSetupSuccess = RTSTH_setup();
		break;
	case Mode::RenderMain_SwapMain:
	case Mode::RenderMain_SwapThread:
	case Mode::RenderThread_SwapThread:
		modeSetupSuccess = true;
		break;
	}

	if (!modeSetupSuccess) {
		CX::Instances::Log.error("CX_AVP") << "setup(): Setup failed.";
	}

	return modeSetupSuccess;
}


void CX_AVP::_configureDomainSync(void) {

	const SyncConfig& syncConfig = _config.sync;

	_domainSync.clearDataClients();

	{
		Sync::DataClient::Configuration dcc;

		dcc.autoUpdate = true; // ?
		dcc.dataContainer = &_config.display->swapData;
		dcc.dataCollectionDuration = syncConfig.requiredSwapDuration;
		dcc.swapPeriodTolerance = syncConfig.displayTolerance;

		if (!_dispClient.setup(dcc)) {
			CX::Instances::Log.error("CX_AVP") << "setup(): Error setting up display swap data client.";
		}

		// why not just configure the member client?
		//if (!_config.display->swapClient.setup(dcc)) {
			// error
		//}
	}

	{
		Sync::DataClient::Configuration sscc;

		sscc.autoUpdate = true;
		sscc.dataContainer = &_config.soundStream->swapData;
		sscc.dataCollectionDuration = syncConfig.requiredSwapDuration;
		sscc.swapPeriodTolerance = syncConfig.soundTolerance;

		if (!_ssClient.setup(sscc)) {
			CX::Instances::Log.error("CX_AVP") << "setup(): Error setting up sound stream swap data client.";
		}

	}

	_domainSync.addDataClient("disp", &_dispClient);
	_domainSync.addDataClient("ss", &_ssClient);

}

CX_SoundBufferPlayer* CX_AVP::getSoundBufferPlayer(void) {
	return &_soundPlayer;
}

bool CX_AVP::releaseRenderingContext(void) {
	// If not calling from main thread, 
	if (!Private::State->glfwContextManager.isMainThread()) {
		Instances::Log.warning("CX_AVP") << "releaseRenderingContext() called from a non-main thread.";
		return Private::State->glfwContextManager.isLockedByMainThread();
	}

	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _displayThread->enableFrameQueue(false);
}

bool CX_AVP::play(void) {

	if (!startPlaying()) {
		return false;
	}

	while (isPlaying()) {
		Instances::Input.pollEvents();
		//userFun();
		updatePlayback();
		std::this_thread::yield();
	}

	return true;
}

// User-facing functions

bool CX_AVP::startPlaying(void) {
	if (slides.size() == 0) {
		return false;
	}

	switch (_config.mode) {
	case Mode::RenderMain_SwapMain:
		return RMSM_startPlaying();
	case Mode::RenderMain_SwapThread:
		return RMST_startPlaying();
	case Mode::RenderThread_SwapThread:
		return RTST_startPlaying();
	case Mode::RM_SM_Helped:
		return RMSMH_startPlaying();
	case Mode::RM_ST_Helped:
		return RMSTH_startPlaying();
	case Mode::RT_ST_Helped:
		return RTSTH_startPlaying();
	}
	return false;
}

bool CX_AVP::isPlaying(void) {
	switch (_config.mode) {
	case Mode::RenderMain_SwapMain:
		return RMSM_isPlaying();
	case Mode::RenderMain_SwapThread:
		return RMST_isPlaying();
	case Mode::RenderThread_SwapThread:
		return RTST_isPlaying();
	case Mode::RM_SM_Helped:
		return RMSMH_isPlaying();
	case Mode::RM_ST_Helped:
		return RMSTH_isPlaying();
	case Mode::RT_ST_Helped:
		return RTSTH_isPlaying();
	}
	return false;
}

void CX_AVP::updatePlayback(void) {
	switch (_config.mode) {
	case Mode::RenderMain_SwapMain:
		return RMSM_updatePlayback();
	case Mode::RenderMain_SwapThread:
		return RMST_updatePlayback();
	case Mode::RenderThread_SwapThread:
		return RTST_updatePlayback();
	case Mode::RM_SM_Helped:
		return RMSMH_updatePlayback();
	case Mode::RM_ST_Helped:
		return RMSTH_updatePlayback();
	case Mode::RT_ST_Helped:
		return RTSTH_updatePlayback();
	}
}

bool CX_AVP::stopPlaying(void) {
	switch (_config.mode) {
	case Mode::RenderMain_SwapMain:
		return RMSM_stopPlaying();
	case Mode::RenderMain_SwapThread:
		return RMST_stopPlaying();
	case Mode::RenderThread_SwapThread:
		return RTST_stopPlaying();
	case Mode::RM_SM_Helped:
		return RMSMH_stopPlaying();
	case Mode::RM_ST_Helped:
		return RMSTH_stopPlaying();
	case Mode::RT_ST_Helped:
		return RTSTH_stopPlaying();
	}
	return false;
}



///////////////////////////////
// Mode::RenderMain_SwapMain //
///////////////////////////////

bool CX_AVP::RMSM_startPlaying(void) {

	_soundPlayer.setSoundBuffer(&sounds);
		
	// Start swapping display buffers
	CX_Millis endTime = Instances::Clock.now() + _config.sync.readyTimeout;
	while (endTime > Instances::Clock.now()) {

		_config.display->swapBuffers();

		if (_dispClient.allReady()) {
			break;
		}
	}

	if (!_domainSync.allReady()) {
		Instances::Log.error("CX_AVP") << "startPlaying(): Display or sound not ready!";
		return false;
	}


	Sync::SyncPoint sp = _All_getDesiredStartSyncPoint();

	if (!sp.valid()) {
		Instances::Log.error("CX_AVP") << "startPlaying(): Unable to get valid sync point.";
		return false;
	}

	_soundPlayer.queuePlayback(sp.clientData["ss"].pred.prediction(), true);

	CX_SlideBufferPlaybackHelper::Configuration helperCfg;
	helperCfg.display = _config.display;
	helperCfg.slideBuffer = &this->slides;
	_slideHelper.setup(helperCfg);

	_slideHelper.startPlaying();
	_slideHelper.setIntendedStartFramesUsingTimeDurations(sp.clientData["disp"].pred.prediction(), _config.display->getFramePeriod());

	_slideHelper.getNextSlide()->intended.startTime = sp.time.prediction();
	_slideHelper.renderNextSlide();

	return true;
}

bool CX_AVP::RMSM_isPlaying(void) {
	return _slideHelper.isPlaying() || _soundPlayer.isPlayingOrQueued();
}

void CX_AVP::RMSM_updatePlayback(void) {

	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_slideHelper.updatePlayback();

	if (!_slideHelper.isPlaying()) {
		return;
	}

	Sync::TimePrediction nextSwapPred = _dispClient.predictNextSwapTime();
	if (!nextSwapPred.usable) {
		Instances::Log.warning("CX_AVP") << "updatePlayback(): Next swap time prediction was not usable. Best effort means continuing anyway.";
	}

	CX_Millis shouldSwapTime = nextSwapPred.lowerBound() - _config.preSwapSafetyBuffer;
	bool shouldSwap = Instances::Clock.now() >= shouldSwapTime;

	if (shouldSwap) {
		_config.display->swapBuffers();

		//Sync::SwapData newest = _dispSwapListener->getNewestData();
		//_slideHelper.bufferSwap(newest.time, newest.unit);

		_RM_postSwapCheck();
	}

}

bool CX_AVP::RMSM_stopPlaying(void) {
	_slideHelper.stopPlaying();
	return true;
}



void CX_AVP::_RM_postSwapCheck(void) {
	//if (!_slideHelper.isPresenting()) {
	//	return;
	//}

	CX_SlideBuffer::Slide* nextSlide = _slideHelper.getNextSlide();
	CX_SlideBuffer::Slide* currentSlide = _slideHelper.getCurrentSlide();

	//if (!nextSlide) {
	//	return;
	//}

	bool atBeginning = !currentSlide && nextSlide;

	Sync::SwapData newest = _config.display->swapData.getLastSwapData();
	_slideHelper.bufferSwap(newest.time, newest.unit);

	// or do this regardless of beginning
	if (atBeginning && _slideHelper.slideAdvancedOnLastSwap()) {

		// it isn't clear that you need this
		_slideHelper.setIntendedStartTimesOfRemainingSlidesFromCurrentSlide();
		_slideHelper.setIntendedStartFramesOfRemainingSlidesFromCurrentSlide();
	}

	FrameNumber nextFrameNumber = _config.display->getLastFrameNumber() + 1;
	nextSlide = _slideHelper.getNextSlide(); // advance slide pointer

	// note <= in frame number so that if somehow a frame is missed, it still renders something next
	if (nextSlide && nextSlide->intended.startFrame <= nextFrameNumber) {
		_slideHelper.renderNextSlide();
	}

	//if (nextSlide && nextSlide->intended.startFrame < nextFrameNumber) {
	//	Instances::Log() << "Not yet";
	//}

}




/////////////////////////////////
// Mode::RenderMain_SwapThread //
/////////////////////////////////

bool CX_AVP::RMST_startPlaying(void) {

	std::lock_guard<std::recursive_mutex> lock(_mutex);

	if (!_displayThread->isThreadRunning()) {
		_displayThread->startThread();
		Instances::Log.notice("CX_AVP") << "startPlaying(): Display thread was not running. It was started.";
	}

	if (!_domainSync.waitUntilAllReady(_config.sync.readyTimeout)) {
		Instances::Log.error("CX_AVP") << "startPlaying(): Display or sound not ready!";
		return false;
	}

	_soundPlayer.setSoundBuffer(&sounds);

	// give yourself at least 5 milliseconds or a whole frame
	if (_dispClient.predictTimeToNextSwap().lowerBound() < CX_Millis(5)) {
		_dispSwapListener->waitForSwap(CX_Millis(200), true);
	}
	_dispSwapListener->hasSwappedSinceLastCheck(); // make sure to reset swap check


	// Get a sync point to get the start sample frame
	Sync::SyncPoint sp = _All_getDesiredStartSyncPoint();

	if (!sp.valid()) {
		Instances::Log.error("CX_AVP") << "startPlaying(): Unable to get valid sync point.";
		return false;
	}

	_soundPlayer.queuePlayback(sp.clientData["ss"].pred.prediction(), true);
	_slideHelper.setIntendedStartFramesUsingTimeDurations(sp.clientData["disp"].pred.prediction(), _config.display->getFramePeriod());

	_slideHelper.startPlaying();

	return true;
}
	
Sync::SyncPoint CX_AVP::_All_getDesiredStartSyncPoint(void) {
	CX_Millis soundTotalBufferLatency = _config.soundStream->getLatencyPerBuffer() * _config.soundStream->getConfiguration().streamOptions.numberOfBuffers;
	// add in other sound latency?

	CX_Millis framePeriod = _config.display->getFramePeriod();

	FrameNumber leadFrames = 1 + ceil(soundTotalBufferLatency / framePeriod) + _config.displayExtraLeadFrames;
	FrameNumber playbackStartFrame = _config.display->getLastFrameNumber() + leadFrames;

	// Get a sync point to get the start sample frame
	Sync::SyncPoint sp = _domainSync.getSyncPoint("disp", playbackStartFrame);

	if (!sp.valid()) {
		Instances::Log.error("CX_AVP") << "startPlaying(): Unable to get valid sync point.";
	}

	return sp;
}

CX_AVP::DesiredStart CX_AVP::_All_getDesiredStart(void) {

	CX_Millis soundTotalBufferLatency = _config.soundStream->getLatencyPerBuffer() * _config.soundStream->getConfiguration().streamOptions.numberOfBuffers;
	// add in other sound latency?

	CX_Millis framePeriod = _config.display->getFramePeriod();

	FrameNumber leadFrames = 1 + ceil(soundTotalBufferLatency / framePeriod) + _config.displayExtraLeadFrames;
	FrameNumber playbackStartFrame = _config.display->getLastFrameNumber() + leadFrames;

	// Get a sync point to get the start sample frame
	Sync::SyncPoint sp = _domainSync.getSyncPoint("disp", playbackStartFrame);

	DesiredStart ds;

	ds.valid = sp.valid();
	ds.time = sp.time.prediction();
	ds.frameNumber = sp.clientData["disp"].pred.prediction();
	ds.sampleFrame = sp.clientData["ss"].pred.prediction();

	return ds;

}

bool CX_AVP::RMST_isPlaying(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _soundPlayer.isPlayingOrQueued() || _slideHelper.isPlaying();
}

void CX_AVP::RMST_updatePlayback(void) {

	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_slideHelper.updatePlayback();

	if (!_slideHelper.isPlaying()) {
		return;
	}

	if (_dispSwapListener->hasSwappedSinceLastCheck()) {
		_RM_postSwapCheck();
	}

}
bool CX_AVP::RMST_stopPlaying(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_slideHelper.stopPlaying();

	_soundPlayer.stop();

	return true;
}




///////////////////////////////////
// Mode::RenderThread_SwapThread //
///////////////////////////////////

bool CX_AVP::RTST_startPlaying(void) {

	std::lock_guard<std::recursive_mutex> lock(_mutex);


	if (!_displayThread->isThreadRunning()) {
		_displayThread->startThread();
		Instances::Log.notice("CX_AVPresenter") << "startPlaying(): Display thread was not running. It was started.";
	}

	//Instances::Clock.sleep(1000); // Consistently works

	if (!_displayThread->frameQueueEnabled()) {
		if (_displayThread->enableFrameQueue(true)) {
			Instances::Log.notice("CX_AVPresenter") << "startPlaying(): Frame queue was disabled. It was enabled.";
		} else {
			Instances::Log.error("CX_AVPresenter") << "startPlaying(): Frame queue could not be enabled.";
			return false;
		}
	}


	if (!_domainSync.waitUntilAllReady(_config.sync.readyTimeout)) {
		Instances::Log.error("CX_AVP") << "startPlaying(): Display or sound not ready!";
		return false;
	}

	_soundPlayer.setSoundBuffer(&sounds);


	// give yourself at least 5 milliseconds or a whole frame
	if (_dispClient.predictTimeToNextSwap().lowerBound() < CX_Millis(5)) {
		_dispSwapListener->waitForSwap(CX_Millis(100), true);
	}
	_dispSwapListener->hasSwappedSinceLastCheck(); // make sure to reset swap check


	Sync::SyncPoint sp = _All_getDesiredStartSyncPoint();

	if (!sp.valid()) {
		Instances::Log.error("CX_AVP") << "startPlaying(): Unable to get valid sync point.";
		return false;
	}

	_soundPlayer.queuePlayback(sp.clientData["ss"].pred.prediction(), true);
	_slideHelper.setIntendedStartFramesUsingTimeDurations(sp.clientData["disp"].pred.prediction(), _config.display->getFramePeriod());


	// Queue frames with display thread
	for (unsigned int i = 0; i < slides.size(); i++) {

		CX_SlideBuffer::Slide* slide = slides.getSlide(i);

		std::shared_ptr<CX_DisplayThread::QueuedFrame> qf = std::make_shared<CX_DisplayThread::QueuedFrame>();
		qf->startFrame = slide->intended.startFrame;

		qf->fbo = slide->framebuffer;
		qf->fun = slide->drawingFunction;

		qf->frameCompleteCallback = std::bind(&CX_AVP::RTST_queuedFrameCompleteCallback, this, std::placeholders::_1);

		if (!_displayThread->queueFrame(qf)) {
			Instances::Log.error("CX_AVPresenter") << "startPlaying(): Failure to queue slide " << i << " with name " << slide->name << ".";
		}

	}

	if (_dispSwapListener->hasSwappedSinceLastCheck()) {
		// warning: didn't complete quickly enough
		Instances::Log.warning("CX_AVPresenter") << "startPlaying(): A display frame swap occurred before playback was fully queued.";
	} else {
		Instances::Log.notice("CX_AVPresenter") << "startPlaying(): Playback queuing completed with " <<
			_config.display->swapClient.predictTimeToNextSwap().pred.millis() << " milliseconds remaining before swap.";
	}

	Instances::Log.flush();

	//_playing = true;
	return true;

}

bool CX_AVP::RTST_isPlaying(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _soundPlayer.isPlayingOrQueued() || _displayThread->getQueuedFrameCount() > 0;
}

void CX_AVP::RTST_updatePlayback(void) {
	return;
}

bool CX_AVP::RTST_stopPlaying(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_displayThread->clearQueuedFrames();

	_soundPlayer.stop();

	return true;
}

void CX_AVP::RTST_queuedFrameCompleteCallback(CX_DisplayThread::QueuedFrameResult&& qfr) {

	std::lock_guard<std::recursive_mutex> lock(_mutex);

	int slideInd = 0;
	for (; slideInd < (int)slides.size(); slideInd++) {
		if (slides.getSlide(slideInd)->intended.startFrame == qfr.desiredStartFrame) {
			break;
		}
	}

	CX_SlideBuffer::Slide* slide = slides.getSlide(slideInd);
	if (slide == nullptr) {
		// fail
		Instances::Log.warning("CX_AVPresenter") << "While completing queued slide presentation, the slide was not found.";
		return;
	}

	slide->actual.startTime = qfr.startTime;
	slide->actual.startFrame = qfr.actualStartFrame;

	if (qfr.renderTimeValid) {
		slide->presInfo.renderCompleteTime = qfr.renderCompleteTime;
	} else {
		slide->presInfo.renderCompleteTime = CX_Millis(-1);
	}

	if (slideInd - 1 >= 0) {
		CX_SlideBuffer::Slide* prevSlide = slides.getSlide(slideInd - 1);
		if (prevSlide) {
			prevSlide->actual.timeDuration = slide->actual.startTime - prevSlide->actual.startTime;
			prevSlide->actual.frameDuration = slide->actual.startFrame - prevSlide->actual.startFrame;
		}
	}

	if (slideInd == slides.size() - 1) {
		// is last slide
		slide->actual.frameDuration = std::numeric_limits<uint64_t>::max();
		slide->actual.timeDuration = CX_Millis::max();
	}

}

////////////////////////
// Mode::RM_SM_Helped //
////////////////////////

bool CX_AVP::RMSMH_setup(void) {

	// _threadUpdateEventHelper is not used by RM_SM_Helped
	_threadUpdateEventHelper.stopListening();

	{
		Util::DisplaySwapper::Configuration cfg;

		cfg.display = _config.display;
		cfg.client = &_dispClient;

		cfg.mode = Util::DisplaySwapper::Mode::Prediction;

		cfg.preSwapSafetyBuffer = _config.preSwapSafetyBuffer;

		if (!_dispSwapper.setup(cfg)) {
			return false;
		}
	}

	
	{
		CX_SlideBufferPredicatePlayback::Configuration ppc;

		ppc.display = _config.display;
		ppc.slideBuffer = &this->slides;

		ppc.deallocateCompletedSlides = false;
		ppc.propagateDelays = true;

		ppc.shouldSwapPredicate = std::bind(&CX_AVP::SM_shouldSwapPredicate, this);
		ppc.hasSwappedPredicate = nullptr;

		ppc.renderNextPredicate = std::bind(&CX_AVP::FrameCounted_renderNextPredicate, this, std::placeholders::_1);

		ppc.reRenderCurrentPredicate = nullptr; // ?

		if (!_slidePP.setup(ppc)) {
			return false;
		}
	}

	return true;
}

bool CX_AVP::RMSMH_startPlaying(void) {

	_soundPlayer.setSoundBuffer(&sounds);

	if (!SM_swapDisplayUntilReady()) {
		Instances::Log.error("CX_AVP") << "startPlaying(): Display or sound not ready!";
		return false;
	}

	DesiredStart desiredStart = _All_getDesiredStart();
	CX_Millis syncPointTimestamp = Instances::Clock.now();
	if (!desiredStart.valid) {
		Instances::Log.error("CX_AVP") << "startPlaying(): Unable to get valid desired start.";
		return false;
	}

	Instances::Log.notice("CX_AVP") << "startPlaying(): At " << syncPointTimestamp << ", start queued for " << desiredStart.time << ".";
	
	_soundPlayer.queuePlayback(desiredStart.sampleFrame, true);

	CX_SlideBufferPredicatePlayback::StartConfig startConfig;
	startConfig.intendedStartTime = desiredStart.time;
	startConfig.intendedStartFrame = desiredStart.frameNumber;

	_slidePP.startPlaying(startConfig);

	//CX_SlideBufferPredicatePlayback::SlideHelperLP lhp = _slidePP.getLockedHelperPointer();
	//lhp->renderNextSlide();

	return true;

}

bool CX_AVP::RMSMH_isPlaying(void) {
	return _slidePP.isPlaying();
}

void CX_AVP::RMSMH_updatePlayback(void) {
	_slidePP.updatePlayback();
}

bool CX_AVP::RMSMH_stopPlaying(void) {
	_slidePP.stopPlaying();
	return true;
}

bool CX_AVP::FrameCounted_renderNextPredicate(const CX_SlideBufferPredicatePlayback::PredicateArgs& args) {
	if (!args.hasSwapped) {
		return false;
	}

	FrameNumber nextFrameNumber = _config.display->swapData.getNextSwapUnit();

	CX_SlideBufferPredicatePlayback::SlideHelperLP lhp = _slidePP.getLockedHelperPointer();
	CX_SlideBuffer::Slide* nextSlide = lhp->getNextSlide();

	// leq on frame number? warn on less than?
	if (nextSlide && nextSlide->intended.startFrame <= nextFrameNumber) {
		if (nextSlide->intended.startFrame < nextFrameNumber) {
			Instances::Log.error("CX_AVP") << "renderNextPredicate(): Slide named \"" << nextSlide->name <<
				"\" had an intended start frame of " << nextSlide->intended.startFrame <<
				" but an actual start frame of " << nextFrameNumber << ".";
		}
		return true;
	}

	return false;
}

bool CX_AVP::Timed_renderNextPredicate(const CX_SlideBufferPredicatePlayback::PredicateArgs& args) {

	CX_Millis minStartTime = _config.display->getLastSwapTime();

	CX_SlideBufferPredicatePlayback::SlideHelperLP lhp = _slidePP.getLockedHelperPointer();
	CX_SlideBuffer::Slide* nextSlide = lhp->getNextSlide();

	if (nextSlide && nextSlide->intended.startTime >= minStartTime) {
		return true;
	}

	return false;
}

bool CX_AVP::SM_shouldSwapPredicate(void) {
	return _dispSwapper.shouldSwap();
}

bool CX_AVP::ST_hasSwappedPredicate(void) {
	return _dispSwapListener->hasSwappedSinceLastCheck();
}



bool CX_AVP::SM_swapDisplayUntilReady(void) {
	CX_Millis endTime = Instances::Clock.now() + _config.sync.readyTimeout;
	while (endTime > Instances::Clock.now()) {

		_config.display->swapBuffers();

		if (_domainSync.allReady()) {
			break;
		}
	}

	return _domainSync.allReady();
}

////////////////////////
// Mode::RM_ST_Helped //
////////////////////////

bool CX_AVP::RMSTH_setup(void) {
	
	CX_SlideBufferPredicatePlayback::Configuration ppc;

	ppc.display = _config.display;
	ppc.slideBuffer = &this->slides;

	ppc.deallocateCompletedSlides = false;
	ppc.propagateDelays = true;


	ppc.shouldSwapPredicate = nullptr;
	ppc.hasSwappedPredicate = std::bind(&CX_AVP::ST_hasSwappedPredicate, this);

	ppc.renderNextPredicate = std::bind(&CX_AVP::FrameCounted_renderNextPredicate, this, std::placeholders::_1);

	ppc.reRenderCurrentPredicate = nullptr; // ?

	if (!_slidePP.setup(ppc)) {
		return false;
	}
	
	auto threadUpdateFunction = [this]() {
		_slidePP.updatePlaybackSwapping();
	};

	_threadUpdateEventHelper.setup(&_displayThread->updateEvent, threadUpdateFunction);

	return true;
}

bool CX_AVP::RMSTH_startPlaying(void) {

	std::lock_guard<std::recursive_mutex> lock(_mutex);

	if (!_displayThread->isThreadRunning()) {
		_displayThread->startThread();
		Instances::Log.notice("CX_AVP") << "startPlaying(): Display thread was not running. It was started.";
	}

	if (!_domainSync.waitUntilAllReady(_config.sync.readyTimeout)) {
		Instances::Log.error("CX_AVP") << "startPlaying(): Display or sound not ready!";
		return false;
	}

	_soundPlayer.setSoundBuffer(&sounds);

	// give yourself at least 5 milliseconds or a whole frame
	if (_dispClient.predictTimeToNextSwap().lowerBound() < CX_Millis(5)) {
		_dispSwapListener->waitForSwap(CX_Millis(200), true);
	}
	_dispSwapListener->hasSwappedSinceLastCheck(); // make sure to reset swap check

	DesiredStart desiredStart = _All_getDesiredStart();

	if (!desiredStart.valid) {
		Instances::Log.error("CX_AVP") << "startPlaying(): Unable to get valid sync point.";
		return false;
	}

	_soundPlayer.queuePlayback(desiredStart.sampleFrame, true);

	CX_SlideBufferPredicatePlayback::StartConfig startConfig;
	startConfig.intendedStartTime = desiredStart.time;
	startConfig.intendedStartFrame = desiredStart.frameNumber;

	_slidePP.startPlaying(startConfig);

	return true;

}

/*
void CX_AVP::ST_something(CX_Millis desiredTime, CX_Millis timeout) {

	// give yourself at least 5 milliseconds or a whole frame
	if (_dispClient.predictTimeToNextSwap().lowerBound() < desiredTime) {
		_dispSwapListener->waitForSwap(timeout, true);
	}
	_dispSwapListener->hasSwappedSinceLastCheck(); // make sure to reset swap check

}
*/

bool CX_AVP::RMSTH_isPlaying(void) {
	return _slidePP.isPlaying();
}

void CX_AVP::RMSTH_updatePlayback(void) {
	_slidePP.updatePlaybackRendering();
}

bool CX_AVP::RMSTH_stopPlaying(void) {
	_slidePP.stopPlaying();
	return true;
}

////////////////////////
// Mode::RT_ST_Helped //
////////////////////////

bool CX_AVP::RTSTH_setup(void) {

	CX_SlideBufferPredicatePlayback::Configuration ppc;

	ppc.display = _config.display;
	ppc.slideBuffer = &this->slides;

	ppc.deallocateCompletedSlides = false;
	ppc.propagateDelays = true;


	ppc.shouldSwapPredicate = nullptr;
	ppc.hasSwappedPredicate = std::bind(&CX_AVP::ST_hasSwappedPredicate, this);

	ppc.renderNextPredicate = std::bind(&CX_AVP::FrameCounted_renderNextPredicate, this, std::placeholders::_1);

	ppc.reRenderCurrentPredicate = nullptr; // ?

	if (!_slidePP.setup(ppc)) {
		return false;
	}


	auto threadUpdateFunction = [this]() {
		_slidePP.updatePlaybackSwapping();
		_slidePP.updatePlaybackRendering();
	};

	_threadUpdateEventHelper.setup(&_displayThread->updateEvent, threadUpdateFunction);

	return true;
}

bool CX_AVP::RTSTH_startPlaying(void) {

	std::lock_guard<std::recursive_mutex> lock(_mutex);

	if (!_displayThread->isThreadRunning()) {
		_displayThread->startThread();
		Instances::Log.notice("CX_AVP") << "startPlaying(): Display thread was not running. It was started.";
	}

	if (!_displayThread->frameQueueEnabled()) {
		if (_displayThread->enableFrameQueue(true)) {
			Instances::Log.notice("CX_AVP") << "startPlaying(): Frame queue was disabled. It was enabled.";
		} else {
			Instances::Log.error("CX_AVP") << "startPlaying(): Frame queue could not be enabled.";
			return false;
		}
	}

	if (!_domainSync.waitUntilAllReady(_config.sync.readyTimeout)) {
		Instances::Log.error("CX_AVP") << "startPlaying(): Display or sound not ready!";
		return false;
	}

	_soundPlayer.setSoundBuffer(&sounds);

	// give yourself at least 5 milliseconds or a whole frame
	if (_dispClient.predictTimeToNextSwap().lowerBound() < CX_Millis(5)) {
		_dispSwapListener->waitForSwap(CX_Millis(200), true);
	}
	_dispSwapListener->hasSwappedSinceLastCheck(); // make sure to reset swap check

	DesiredStart desiredStart = _All_getDesiredStart();

	if (!desiredStart.valid) {
		Instances::Log.error("CX_AVP") << "startPlaying(): Unable to get valid sync point.";
		return false;
	}

	_soundPlayer.queuePlayback(desiredStart.sampleFrame, true);

	CX_SlideBufferPredicatePlayback::StartConfig startConfig;
	startConfig.intendedStartTime = desiredStart.time;
	startConfig.intendedStartFrame = desiredStart.frameNumber;

	_slidePP.startPlaying(startConfig);

	return true;
}

bool CX_AVP::RTSTH_isPlaying(void) {
	return _slidePP.isPlaying();
}

void CX_AVP::RTSTH_updatePlayback(void) {
	return;
}

bool CX_AVP::RTSTH_stopPlaying(void) {
	_slidePP.stopPlaying();
	return true;
}

} // namespace CX