#include "CX_SwappingThread.h"

#include "ofAppGLFWWindow.h" //This is included here in order to avoid leaking symbols. 
//There are a lot of symbols in this file that should not leak into the user space.

#include "CX_Private.h" //glfwContext

using namespace CX;

CX_ConstantlySwappingThread::CX_ConstantlySwappingThread (void) :
	_frameCount(0),
	_frameCountOnLastCheck(0),
	_isLocked(false)
{
}

void CX_ConstantlySwappingThread::threadedFunction (void) {
	while (isThreadRunning()) {
		//yield();
		glfwSwapBuffers( CX::Private::glfwContext );

		CX_Micros swapTime = CX::Instances::Clock.getTime();

		if (lock()) {
			++_frameCount;

			_recentSwapTimes.push_back( swapTime );
			while (_recentSwapTimes.size() > 30) {
				_recentSwapTimes.pop_front();
			}

			unlock();
		}
	}
}

bool CX_ConstantlySwappingThread::swappedSinceLastCheck (void) {
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

CX_Micros CX_ConstantlySwappingThread::getTypicalSwapPeriod (void) {
	CX_Micros typicalSwapPeriod = 0;
	if (_lockMutex()) {
		if (_recentSwapTimes.size() >= 2) {
			CX_Micros swapPeriodSum = 0;
			for (unsigned int i = 1; i < _recentSwapTimes.size(); i++) {
				swapPeriodSum += _recentSwapTimes[i] - _recentSwapTimes[i - 1];
			}
			typicalSwapPeriod = swapPeriodSum/(_recentSwapTimes.size() - 1);
		}
		_unlockMutex();
	}
	return typicalSwapPeriod;
}

CX_Micros CX_ConstantlySwappingThread::estimateNextSwapTime (void) {
	CX_Micros nextSwapTime = 0;
	if (_lockMutex()) {
		if (_recentSwapTimes.size() >= 2) {
			nextSwapTime = _recentSwapTimes.back() + getTypicalSwapPeriod();
		}
		_unlockMutex();
	}
	return nextSwapTime;
}

CX_Micros CX_ConstantlySwappingThread::getLastSwapTime (void) {
	CX_Micros lastSwapTime = 0;
	if (_lockMutex()) {
		if (_recentSwapTimes.size() > 0) {
			lastSwapTime = _recentSwapTimes.back();
		}
		_unlockMutex();
	}
	return lastSwapTime;
}

CX_Micros CX_ConstantlySwappingThread::getLastSwapPeriod (void) {
	CX_Micros lastSwapPeriod = 0;
	if (_lockMutex()) {
		if (_recentSwapTimes.size() >= 2) {
			lastSwapPeriod = _recentSwapTimes.at( _recentSwapTimes.size() - 1 ) - _recentSwapTimes.at( _recentSwapTimes.size() - 2 );
		}
		_unlockMutex();
	}
	return lastSwapPeriod;
}

uint64_t CX_ConstantlySwappingThread::getFrameNumber (void) {
	uint64_t frameNumber = 0;
	if (_lockMutex()) {
		frameNumber = _frameCount;
		_unlockMutex();
	}
	return frameNumber;
}


bool CX_ConstantlySwappingThread::_lockMutex (void) {
	if (_isLocked) {
		return true;
	}

	if (lock()) {
		_isLocked = true;
		return true;
	}
	return false;
}

bool CX_ConstantlySwappingThread::_unlockMutex (void) {
	_isLocked = false;
	unlock();
	return true;
}