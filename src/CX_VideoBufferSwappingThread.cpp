#include "CX_VideoBufferSwappingThread.h"

#include "CX_Private.h" //glfwContext

using namespace CX;

CX_VideoBufferSwappingThread::CX_VideoBufferSwappingThread (void) :
	_frameCount(0),
	_frameCountOnLastCheck(0),
	_isLocked(false),
	_swapsBeforeStop(-1),
	_glFinishAfterSwap(true)
{
}

void CX_VideoBufferSwappingThread::threadedFunction (void) {
	while (isThreadRunning()) {
		
		glfwSwapBuffers( CX::Private::glfwContext );
		//if (lock() && _glFinishAfterSwap) {
		//	unlock();
			glFinish();
		//}

		CX_Millis swapTime = CX::Instances::Clock.now();

		if (lock()) {
			++_frameCount;

			_recentSwapTimes.push_back( swapTime );
			while (_recentSwapTimes.size() > 30) {
				_recentSwapTimes.pop_front();
			}

			bool stopSwapping = false;
			if (_swapsBeforeStop > 0) {
				if (--_swapsBeforeStop == 0) {
					stopSwapping = true;
				}
			}

			unlock();

			if (stopSwapping) {
				this->stopThread();
			}
		}
		//yield(); //??
	}
}


void CX_VideoBufferSwappingThread::swapNFrames (unsigned int n) {
	if (n == 0) {
		return;
	}

	if (!this->isThreadRunning()) {
		_swapsBeforeStop = n;
		this->startThread(true, false);
	} else {
		if (_lockMutex()) {
			_swapsBeforeStop = n;
			_unlockMutex();
		}
	}
}

bool CX_VideoBufferSwappingThread::swappedSinceLastCheck (void) {
	bool rval = false;
	if (_lockMutex()) {
		if (_frameCount != _frameCountOnLastCheck) {
			_frameCountOnLastCheck = _frameCount;
			rval = true;
		}
		_unlockMutex();
	}
	return rval;
}

CX_Millis CX_VideoBufferSwappingThread::getTypicalSwapPeriod(void) {
	CX_Millis typicalSwapPeriod = 0;
	if (_lockMutex()) {
		if (_recentSwapTimes.size() >= 2) {
			CX_Millis swapPeriodSum = 0;
			for (unsigned int i = 1; i < _recentSwapTimes.size(); i++) {
				swapPeriodSum += _recentSwapTimes[i] - _recentSwapTimes[i - 1];
			}
			typicalSwapPeriod = swapPeriodSum/(_recentSwapTimes.size() - 1);
		}
		_unlockMutex();
	}
	return typicalSwapPeriod;
}

CX_Millis CX_VideoBufferSwappingThread::estimateNextSwapTime(void) {
	CX_Millis nextSwapTime = 0;
	if (_lockMutex()) {
		if (_recentSwapTimes.size() >= 2) {
			nextSwapTime = _recentSwapTimes.back() + getTypicalSwapPeriod();
		}
		_unlockMutex();
	}
	return nextSwapTime;
}

CX_Millis CX_VideoBufferSwappingThread::getLastSwapTime(void) {
	CX_Millis lastSwapTime = 0;
	if (_lockMutex()) {
		if (_recentSwapTimes.size() > 0) {
			lastSwapTime = _recentSwapTimes.back();
		}
		_unlockMutex();
	}
	return lastSwapTime;
}

CX_Millis CX_VideoBufferSwappingThread::getLastSwapPeriod(void) {
	CX_Millis lastSwapPeriod = 0;
	if (_lockMutex()) {
		if (_recentSwapTimes.size() >= 2) {
			lastSwapPeriod = _recentSwapTimes.at( _recentSwapTimes.size() - 1 ) - _recentSwapTimes.at( _recentSwapTimes.size() - 2 );
		}
		_unlockMutex();
	}
	return lastSwapPeriod;
}

uint64_t CX_VideoBufferSwappingThread::getFrameNumber (void) {
	uint64_t frameNumber = 0;
	if (_lockMutex()) {
		frameNumber = _frameCount;
		_unlockMutex();
	}
	return frameNumber;
}

void CX_VideoBufferSwappingThread::setGLFinishAfterSwap(bool finishAfterSwap) {
	if (_lockMutex()) {
		_glFinishAfterSwap = finishAfterSwap;
		_unlockMutex();
	}
}


bool CX_VideoBufferSwappingThread::_lockMutex (void) {
	if (_isLocked) {
		return true;
	}

	if (lock()) {
		_isLocked = true;
		return true;
	}
	return false;
}

bool CX_VideoBufferSwappingThread::_unlockMutex (void) {
	_isLocked = false;
	unlock();
	return true;
}