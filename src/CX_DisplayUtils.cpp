#include "CX_DisplayUtils.h"

#include "CX_Display.h"

namespace CX {
namespace Util {

	std::vector<double> framePeriodToFrameRate(const std::vector<CX_Millis>& periods) {
		std::vector<double> rates(periods.size());

		for (size_t i = 0; i < periods.size(); i++) {
			rates[i] = 1.0 / periods[i].seconds();
		}

		return rates;
	}

	FrameRateEstimationConfig::FrameRateEstimationConfig(void) :
		disp(&CX::Instances::Disp)
	{}


	///////////////////////////
	// FrameEstimationResult //
	///////////////////////////

	void FrameEstimationResult::clear(void) {
		this->success = false;
		this->messages.clear();

		allPeriods.clear();
		includedPeriods.clear();
		excludedPeriods.clear();
	}

	void FrameEstimationResult::filterByFramePeriod(CX_Millis minPeriod, CX_Millis maxPeriod) {

		includedPeriods.clear();
		excludedPeriods.clear();

		for (const CX_Millis& fp : allPeriods) {
			if (minPeriod <= fp && fp <= maxPeriod) {
				includedPeriods.push_back(fp);
			} else {
				excludedPeriods.push_back(fp);
			}
		}

	}

	void FrameEstimationResult::filterByFrameRate(double minRate, double maxRate) {

		CX_Millis minPeriod = CX_Seconds(1.0 / maxRate);
		CX_Millis maxPeriod = CX_Seconds(1.0 / minRate);

		filterByFramePeriod(minPeriod, maxPeriod);
	}

	CX_Millis FrameEstimationResult::calcFramePeriodMean(void) const {
		return CX::Util::mean(this->includedPeriods);
	}

	CX_Millis FrameEstimationResult::calcFramePeriodSD(void) const {
		return CX_Millis::standardDeviation(this->includedPeriods);
	}

	double FrameEstimationResult::calcFrameRateMean(void) const {
		std::vector<double> rates = CX::Util::framePeriodToFrameRate(includedPeriods);
		return CX::Util::mean(rates);

		//return 1.0 / calcFramePeriodMean().seconds();
	}



	FrameEstimationResult estimateFrameRate(const FrameRateEstimationConfig& estCfg) {

		FrameEstimationResult rval;
		rval.success = true; // it can only be set to false in the function

		CX_Display* disp = estCfg.disp;

		bool wasSwapping = disp->isAutomaticallySwapping();
		disp->setAutomaticSwapping(false);

		std::vector<CX_Millis> swapTimes;

		//CX_Millis minFramePeriod = CX_Seconds((1.0 / maxFrameRate));
		//CX_Millis maxFramePeriod = CX_Seconds((1.0 / minFrameRate));

		// For some reason, frame period estimation gets screwed up because the first few swaps are way too fast
		// if the buffers haven't been swapping for some time, so swap a few times to clear out the "bad" initial swaps.
		for (int i = 0; i < 3; i++) {
			disp->swapBuffers();
		}

		CX_Millis startTime = CX::Instances::Clock.now();
		while (CX::Instances::Clock.now() - startTime < estCfg.estimationTime) {
			disp->swapBuffers();
			swapTimes.push_back(CX::Instances::Clock.now());
		}

		if (swapTimes.size() < 3) {
			std::ostringstream oss;

			oss << "Error: Not enough buffer swaps occurred during the " <<
				estCfg.estimationTime.seconds() << 
				" second estimation interval. At least 3 swaps are needed to calculate anything.";

			rval.success = false;
			rval.messages.push_back(oss.str());

			disp->setAutomaticSwapping(wasSwapping);

			return rval;
		}

		//std::vector<CX_Millis> swapPeriods(swapTimes.size() - 1);
		rval.allPeriods.resize(swapTimes.size() - 1);
		for (size_t i = 0; i < rval.allPeriods.size(); i++) {
			rval.allPeriods[i] = swapTimes[i + 1] - swapTimes[i];
		}

		rval.filterByFrameRate(estCfg.minFrameRate, estCfg.maxFrameRate);

		if (rval.includedPeriods.size() < estCfg.minGoodIntervals) {
			
			std::ostringstream oss;
			oss << "Error: Not enough valid swaps occurred during the " <<
				estCfg.estimationTime.millis() << " ms estimation interval. If the estimation interval was very short (less than 50 ms), you "
				"could try making it longer. If the estimation interval was longer, this is an indication that there is something "
				"wrong with the video card configuration. Try using CX_Display::testBufferSwapping() to narrow down the source "
				"of the problems.";

			rval.success = false;
			rval.messages.push_back(oss.str());
		}

		if (rval.excludedPeriods.size() > 0) {

			size_t totalExcluded = rval.excludedPeriods.size();

			size_t usedExcluded = std::min<size_t>(totalExcluded, estCfg.maxBadIntervalsPrinted);
			std::string usedStr = (usedExcluded == totalExcluded) ? "" : (" first " + ofToString(usedExcluded));

			std::vector<double> usedMs(usedExcluded);
			for (size_t i = 0; i < usedExcluded; i++) {
				usedMs[i] = rval.excludedPeriods[i].millis();
			}

			std::ostringstream oss;
			oss << "Warning: " << totalExcluded << " buffer swap durations were " <<
				"outside of the allowed range of " << estCfg.minFrameRate << " to " << estCfg.maxFrameRate << " fps. The" << usedStr <<
				" excluded durations were: " << CX::Util::vectorToString(usedMs, ", ", 5);

			//rval.success = false; // does not cause failure
			rval.messages.push_back(oss.str());
		}

		disp->setAutomaticSwapping(wasSwapping);

		return rval;
	}

	///////////////
	// GLVersion //
	///////////////

	/*! Compare `GLVersion`s.
	\param that `GLVersion` to compare to.
	\return One of:
	+ `this > that: 1`
	+ `this == that: 0`
	+ `this < that: -1`
	*/
	int GLVersion::compare(int maj, int min, int rel) const {
		return compare(GLVersion(maj, min, rel));
	}

	int GLVersion::compare(const GLVersion& that) const {

		if (this->major > that.major) {
			return 1;
		}
		else if (this->major < that.major) {
			return -1;
		}

		if (this->minor > that.minor) {
			return 1;
		}
		else if (this->minor < that.minor) {
			return -1;
		}

		if (this->release > that.release) {
			return 1;
		}
		else if (this->release < that.release) {
			return -1;
		}

		return 0;
	}

	/*! Fence Sync is supported by OpenGL version 3.2.0 and higher.

	\return `true` if this is at least 3.2.0.
	*/
	bool GLVersion::supportsGLFenceSync(void) const {
		return this->compare(3, 2, 0) >= 0;
	}

	// See https://www.khronos.org/opengl/wiki/Core_Language_(GLSL)#OpenGL_and_GLSL_versions
	// Also https://en.wikipedia.org/wiki/OpenGL_Shading_Language#Versions
	GLVersion GLVersion::getCorrespondingGLSLVersion(void) const {
		if (this->major < 2) {
			return GLVersion(0, 0, 0); //No version exists
		}
		else if (this->major == 2 && this->minor == 0) {
			return GLVersion(1, 10, 59);
		}
		else if (this->major == 2 && this->minor == 1) {
			return GLVersion(1, 20, 8);
		}
		else if (this->major == 3 && this->minor == 0) {
			return GLVersion(1, 30, 10);
		}
		else if (this->major == 3 && this->minor == 1) {
			return GLVersion(1, 40, 8);
		}
		else if (this->major == 3 && this->minor == 2) {
			return GLVersion(1, 50, 11);
		}
		else if (this->compare(3, 3, 0) >= 0) {
			return *this;
		}

		return GLVersion(0, 0, 0); //No version exists
	}


///////////////////////////
// GlfwContextManager //
///////////////////////////

void GlfwContextManager::setup(GLFWwindow* context, std::thread::id mainThreadId) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_glfwContext = context;
	_mainThreadId = mainThreadId;

	this->lock();
}

bool GlfwContextManager::trylock(void) {

	std::lock_guard<std::recursive_mutex> lock(_mutex);

	if (isUnlocked()) {
		_lockingThreadId = std::this_thread::get_id();

		// TODO: Look into this for oF 0.10.1
		glfwMakeContextCurrent(_glfwContext);
		//glFinish();

//#if !(OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR >= 10 && OF_VERSION_PATCH >= 1)
		// This extra release and acquire appears to be required, probably due to bug, maybe due to nasty GLFW and oF interaction
		glfwMakeContextCurrent(NULL);
		//glFinish();
		glfwMakeContextCurrent(_glfwContext);
		//glFinish();
//#endif

		return true;
	}

	return false;
}

void GlfwContextManager::lock(void) {
	while (!trylock())
		;
}

void GlfwContextManager::unlock(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	if (isLockedByThisThread()) {
		_lockingThreadId = std::thread::id();
		glfwMakeContextCurrent(NULL);
		//glFinish();
	}
}

bool GlfwContextManager::isUnlocked(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _lockingThreadId == std::thread::id();
}

bool GlfwContextManager::isLockedByThisThread(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _lockingThreadId == std::this_thread::get_id();
}

bool GlfwContextManager::isLockedByMainThread(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _lockingThreadId == _mainThreadId;
}

bool GlfwContextManager::isLockedByAnyThread(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _lockingThreadId != std::thread::id();
}

std::thread::id GlfwContextManager::getLockingThreadId(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _lockingThreadId;
}

GLFWwindow* GlfwContextManager::get(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	GLFWwindow* rval = nullptr;
	if (isLockedByThisThread()) {
		rval = _glfwContext;
	}
	return rval;
}

bool GlfwContextManager::isMainThread(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	return std::this_thread::get_id() == _mainThreadId;
}

//////////////////
// GLSyncHelper //
//////////////////

GLSyncHelper::GLSyncHelper(void) :
	//_syncSuccess(false),
	_syncComplete(-1),
	_syncStart(-1),
	_status(SyncStatus::Idle)
{}

void GLSyncHelper::startSync(CX_Millis timeout) {

	clear();

	_fenceSyncObject = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glFlush(); // This glFlush ensures that the fence sync object gets pushed into the command queue.
	// But its redundant if updateSync calls glClientWaitSync with flag GL_SYNC_FLUSH_COMMANDS_BIT?

	_syncStart = CX::Instances::Clock.now();
	_timeout = timeout;

	_status = SyncStatus::Syncing;
}

void GLSyncHelper::updateSync(void) {

	if (_status != SyncStatus::Syncing) {
		return;
	}

	// GL_SYNC_FLUSH_COMMANDS_BIT can be given as second argument, but docs say not to use it past the first call,
	// and I don't care to do first call tracking, I'll just glFlush()
	GLenum result = glClientWaitSync(_fenceSyncObject, GL_SYNC_FLUSH_COMMANDS_BIT, 0);

	if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {

		_syncComplete = CX::Instances::Clock.now();
		_status = SyncStatus::SyncSuccess;

	}
	else if (result == GL_WAIT_FAILED) {

		// Log something? This is an OpenGL error
		_syncComplete = CX::Instances::Clock.now();
		_status = SyncStatus::SyncFailed;

	}
	//else if (result == GL_TIMEOUT_EXPIRED) {
		//do nothing. This isn't supposed to be able to happen
		//CX::Instances::Log.warning("GLSyncHelper") << "Sync timeout";
	//}

	// Check for timeout after checking the wait sync to give one last
	// chance before timing out.
	if (_timeout > CX_Millis(0)) {
		CX_Millis t = CX::Instances::Clock.now();
		if (t - _syncStart > _timeout) {
			_syncComplete = t;
			_status = SyncStatus::TimedOut;
		}
	}
}

void GLSyncHelper::stopSyncing(void) {
	// This may generate GL_INVALID_VALUE error if _fenceSyncObject is not initialized.
	// But its silent, so you can ignore it.
	glDeleteSync(_fenceSyncObject);

	_status = SyncStatus::Idle;
}

void GLSyncHelper::clear(void) {
	stopSyncing();
	_syncComplete = CX_Millis(-1);
	_syncStart = CX_Millis(-1);
	_timeout = CX_Millis(-1);
}

bool GLSyncHelper::isSyncing(void) const {
	return _status == SyncStatus::Syncing;
}

bool GLSyncHelper::syncSuccess(void) const {
	return _status == SyncStatus::SyncSuccess;
}

bool GLSyncHelper::syncComplete(void) const {
	return _status == SyncStatus::SyncSuccess ||
		_status == SyncStatus::SyncFailed ||
		_status == SyncStatus::TimedOut;
}

GLSyncHelper::SyncStatus GLSyncHelper::getStatus(void) const {
	return _status;
}

CX_Millis GLSyncHelper::getStartTime(void) const {
	return _syncStart;
}

CX_Millis GLSyncHelper::getCompleteTime(void) const {
	return _syncComplete;
}



////////////////////
// DisplaySwapper //
////////////////////

bool DisplaySwapper::setup(const Configuration& config) {
	if (!config.display) {
		return false;
	}

	_config = config;

	if (!_config.client) {
		_config.client = &_config.display->swapClient;
	}

	if (config.preSwapSafetyBuffer < CX_Millis(1)) {
		Instances::Log.warning("DisplaySwapper") <<
			"setup(): config.preSwapSafetyBuffer was less than 1 millisecond. " <<
			"It is recommended that preSwapSafetyBuffer be at least one millisecond.";
		if (_config.preSwapSafetyBuffer < CX_Millis(0)) {
			_config.preSwapSafetyBuffer = CX_Millis(0);
		}
	}

	return true;
}

const DisplaySwapper::Configuration& DisplaySwapper::getConfiguration(void) const {
	return _config;
}


bool DisplaySwapper::shouldSwap(void) const {

	switch (_config.mode) {
	case Mode::NominalPeriod:
		return _NominalPeriod_shouldSwap();
	case Mode::Prediction:
		return _Prediction_shouldSwap();
	}

	return false;
}

// true if swap happened
bool DisplaySwapper::trySwap(void) {

	if (!shouldSwap()) {
		return false;
	}

	_config.display->swapBuffers(); // or do this

	return true;
}

bool DisplaySwapper::_NominalPeriod_shouldSwap(void) const {

	// TODO: cache the value of getFramePeriod somehow?
	CX_Millis nextSwapEst = _config.display->getLastSwapTime() + _config.display->getFramePeriod();

	CX_Millis timeToSwap = nextSwapEst - Instances::Clock.now();

	return timeToSwap < _config.preSwapSafetyBuffer;

}

bool DisplaySwapper::_Prediction_shouldSwap(void) const {
	Sync::TimePrediction tp = _config.client->predictNextSwapTime();

	if (tp.usable) {

		tp.pred -= Instances::Clock.now();

		CX_Millis minTimeToSwap = tp.lowerBound();

		return minTimeToSwap < _config.preSwapSafetyBuffer;

	}

	return _NominalPeriod_shouldSwap();
}


} // namespace Util
} // namespace CX