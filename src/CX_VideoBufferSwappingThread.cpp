#include "CX_VideoBufferSwappingThread.h"

#include "CX_Private.h" //glfwContext

namespace CX {
namespace Private {

void CX_VideoBufferThread::setup(Configuration config, bool startThread) {

	std::lock_guard<std::recursive_mutex> lock(_mutex);

	stop(true);

	_config = config;

	if (startThread) {
		start();
	}

}

const CX_VideoBufferThread::Configuration& CX_VideoBufferThread::getConfiguration(void) {
	return _config;
}

void CX_VideoBufferThread::setSwapContinuously(bool swapContinuously) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_config.swapContinuously = swapContinuously;
}

void CX_VideoBufferThread::setGLFinishAfterSwap(bool glFinishAfterSwap) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_config.glFinishAfterSwap = glFinishAfterSwap;
}

void CX_VideoBufferThread::setSleepTimePerLoop(CX_Millis sleepTime) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_config.sleepTimePerLoop = sleepTime;
}

void CX_VideoBufferThread::setPreSwapCpuHogging(CX_Millis preSwapHogging) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_config.preSwapCpuHogging = preSwapHogging;
}

void CX_VideoBufferThread::setUnlockMutexDuringSwap(bool unlock) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_config.unlockMutexDuringSwap = unlock;
}

void CX_VideoBufferThread::start(void) {

	std::lock_guard<std::recursive_mutex> lock(_mutex);

	if (_threadRunning) {
		return;
	}

	_queuedSwaps = 0;
	
	_frameCount = 0;
	_frameCountOnLastCheck = 0;
	

	_swapLM.setup(true, 60, 3);

	_thread = std::thread(&CX_VideoBufferThread::_threadFunction, this);
	_threadRunning = true;

}

void CX_VideoBufferThread::stop(bool wait) {
	_mutex.lock();

	if (!_threadRunning) {
		_mutex.unlock();
		return;
	}

	_threadRunning = false;
	_mutex.unlock();

	if (wait) {
		_thread.join();
	}
}

bool CX_VideoBufferThread::isRunning(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _threadRunning;
}

void CX_VideoBufferThread::clearSwapHistory(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_swapLM.clear();

	_syncData.clear(); //?
}



bool CX_VideoBufferThread::isSynchronized(bool stopIfSynchronized) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	if (_syncData.size() < _syncConfig.requiredSwaps) {
		return false;
	}

	// Check that all of the last N swaps are within tolerance

	CX_Millis minPeriod = _syncConfig.nominalFramePeriod - _syncConfig.periodTolerance;
	CX_Millis maxPeriod = _syncConfig.nominalFramePeriod + _syncConfig.periodTolerance;

	bool withinTolerance = true;

	for (unsigned int i = 0; i < _syncData.size() - 1; i++) {

		CX_Millis dif = _syncData[i + 1] - _syncData[i];

		bool tol = dif >= minPeriod && dif <= maxPeriod;

		withinTolerance = withinTolerance && tol;
	}

	/*
	if (!_syncLM.modelReady()) {
		return false;
	}

	CX_Millis estFramePeriod = CX_Seconds(_syncLM.getSlope());

	bool withinTolerance = estFramePeriod >= minPeriod && estFramePeriod <= maxPeriod;
	*/

	if (stopIfSynchronized && withinTolerance) {
		stopSynchronizing();
	}

	return withinTolerance;
}

void CX_VideoBufferThread::stopSynchronizing(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_synchronizing = false;

	_syncData.clear();

	_config.swapContinuously = _previous_swapContinuously;
}

// Synchronization continues running
bool CX_VideoBufferThread::synchronize(SynchronizeConfig config, CX_Millis timeout) {

	// Without futures
	startSynchronizing(config);

	CX_Millis timeoutEnd = Instances::Clock.now() + timeout;
	bool syncSuccess = false;
	bool syncing = true;
	
	while (syncing) {

		if (Instances::Clock.now() >= timeoutEnd) {
			syncing = false;
		}

		if (!isSynchronizing()) {
			syncing = false;
		}

		if (isSynchronized(config.stopSynchronizingOnceSynchronized)) {
			syncing = false;
			syncSuccess = true;
		}

	}

	return syncSuccess;

	// With futures
	/*

	_mutex.lock();

	long long timeoutLL = timeout.millis();
	std::chrono::milliseconds timeoutMs(timeoutLL);

	_mutex.unlock();

	std::future<void> fut = startSynchronizing(config);
	if (fut.valid()) {
		fut.wait_for(timeoutMs);
	} else {
		return false;
	}
	
	return isSynchronized(false);
	*/
}

bool CX_VideoBufferThread::startSynchronizing(SynchronizeConfig config) {

	std::lock_guard<std::recursive_mutex> lock(_mutex);

	if (!_threadRunning) {
		if (config.startThreadIfStopped) {
			this->start();
		} else {
			return false;
		}
	}

	_synchronizing = true;
	_syncConfig = config;

	//_syncLM.setup(false, _syncConfig.requiredSwaps, _syncConfig.requiredSwaps);
	_syncData.clear();

	_previous_swapContinuously = _config.swapContinuously;
	_config.swapContinuously = true;

	/*
	auto syncFun = [](CX_VideoBufferThread* caller) -> void {
		do {  } while (caller->isSynchronizing() && !caller->isSynchronized(false));
	};

	std::future<void> fut = std::async(syncFun, this);
	*/

	return true;

}

bool CX_VideoBufferThread::isSynchronizing(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _synchronizing;
}


void CX_VideoBufferThread::queueSwaps(unsigned int n) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	if (_config.swapContinuously) {
		// warn?
		return;
	}

	_queuedSwaps += n;
}

bool CX_VideoBufferThread::hasSwappedSinceLastCheck(bool reset) {
	return swapsSinceLastCheck(reset) > 0;
}

int64_t CX_VideoBufferThread::swapsSinceLastCheck(bool reset) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	int64_t rval = _frameCount - _frameCountOnLastCheck;
	if (reset && _frameCount != _frameCountOnLastCheck) {
		_frameCountOnLastCheck = _frameCount;
	}
	return rval;
}

int64_t CX_VideoBufferThread::getFrameNumber(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _frameCount;
}

CX_Millis CX_VideoBufferThread::getLastSwapTime(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _lastSwapTime;
}

CX_Millis CX_VideoBufferThread::estimateNextSwapTime(void) {
	return estimateLaggedSwapTime(1);
}

// Lag of 0 is the last swap
CX_Millis CX_VideoBufferThread::estimateLaggedSwapTime(int lag) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return CX_Seconds(_swapLM.getY(_frameCount + lag));
}


void CX_VideoBufferThread::_threadFunction(void) {

	bool skipNextSleep = true;

	while (_threadRunning) {

		// Sleep
		if (skipNextSleep) {
			skipNextSleep = false;
		} else {
			_mutex.lock();
			CX_Millis sleepTime = _config.sleepTimePerLoop;
			_mutex.unlock();
			Instances::Clock.sleep(sleepTime);
		}


		_mutex.lock();
		bool shouldSwap = _config.swapContinuously || _queuedSwaps > 0;

		if (_config.preSwapCpuHogging > CX_Millis(0)) {
			CX_Millis estTimeToSwap = estimateNextSwapTime() - Instances::Clock.now();
			shouldSwap = shouldSwap && (estTimeToSwap <= _config.preSwapCpuHogging);
		}

		_mutex.unlock();

		if (shouldSwap) {
			_swap();
			skipNextSleep = true;
		}

	} // while (_threadRunning)

}

void CX_VideoBufferThread::_swap(void) {

	_mutex.lock();

	bool glFinishAfterSwap = _config.glFinishAfterSwap;
	bool unlockMutexDuringSwap = _config.unlockMutexDuringSwap;
	int64_t nextFrameCount = _frameCount + 1;

	if (unlockMutexDuringSwap) {
		_mutex.unlock();
	}

	
	glfwSwapBuffers(CX::Private::glfwContext);
	if (glFinishAfterSwap) {
		glFinish();
	}

	CX_Millis swapTime = CX::Instances::Clock.now();

	VideoSwapData vsd{ swapTime, nextFrameCount };
	ofNotifyEvent(swapEvent, vsd);
	


	if (unlockMutexDuringSwap) {
		_mutex.lock();
	}

	_frameCount = nextFrameCount;
	_lastSwapTime = swapTime;

	_swapLM.store(_frameCount, _lastSwapTime.seconds());
	if (_synchronizing) {
		//_syncLM.store(_frameCount, _lastSwapTime.seconds());

		_syncData.push_back(_lastSwapTime);
		while (_syncData.size() > _syncConfig.requiredSwaps) {
			_syncData.pop_front();
		}
	}

	if (_queuedSwaps > 0) {
		_queuedSwaps--;
	}

	_mutex.unlock();

}

} //namespace Private
} //namespace CX