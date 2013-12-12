#include "CX_SwappingThread.h"

#include "ofAppGLFWWindow.h" //This is included here in order to avoid leaking symbols. 
//There are a lot of symbols in this file that should not leak into the user space.

#include "CX_Utilities.h" //glfwContext

using namespace CX;

CX_ConstantlySwappingThread::CX_ConstantlySwappingThread (void) :
	_frameCount(0),
	_frameCountOnLastCheck(0)
{
	_lastSwapTime = CX::Instances::Clock.getTime();
}

void CX_ConstantlySwappingThread::threadedFunction (void) {
	while (isThreadRunning()) {
		//yield();
		glfwSwapBuffers( CX::Private::glfwContext );

		uint64_t swapTime = CX::Instances::Clock.getTime();

		if (lock()) {
			++_frameCount;

			_recentSwapPeriods.push_back( swapTime - _lastSwapTime );
			while (_recentSwapPeriods.size() > 30) {
				_recentSwapPeriods.pop_front();
			}
			_lastSwapTime = swapTime;

			unlock();
		}
	}
}

bool CX_ConstantlySwappingThread::swappedSinceLastCheck (void) {
	bool rval = false;
	if (lock()) {
		if (_frameCount != _frameCountOnLastCheck) {
			_frameCountOnLastCheck = _frameCount;
			rval = true;
		}
		unlock();
	}
	return rval;
}

uint64_t CX_ConstantlySwappingThread::getTypicalSwapPeriod (void) {
	uint64_t typicalSwapPeriod = 0;
	if (lock()) {
		for (int i = 0; i < _recentSwapPeriods.size(); i++) {
			typicalSwapPeriod += _recentSwapPeriods.at(i);
		}

		typicalSwapPeriod /= _recentSwapPeriods.size();

		unlock();
	}
	return typicalSwapPeriod;
}

uint64_t CX_ConstantlySwappingThread::estimateNextSwapTime (void) {
	uint64_t nextSwapTime = 0;
	if (lock()) {
		uint64_t typicalSwapPeriod = 0;
		for (int i = 0; i < _recentSwapPeriods.size(); i++) {
			typicalSwapPeriod += _recentSwapPeriods.at(i);
		}

		typicalSwapPeriod /= _recentSwapPeriods.size();

		nextSwapTime = _lastSwapTime + typicalSwapPeriod;
		unlock();
	}
	return nextSwapTime;
}

uint64_t CX_ConstantlySwappingThread::getLastSwapTime (void) {
	uint64_t lastSwapTime = 0;
	if (lock()) {
		lastSwapTime = _lastSwapTime;
		unlock();
	}
	return lastSwapTime;
}

uint64_t CX_ConstantlySwappingThread::getLastSwapPeriod (void) {
	uint64_t lastSwapPeriod = 0;
	if (lock()) {
		lastSwapPeriod = _recentSwapPeriods.front();
		unlock();
	}
	return lastSwapPeriod;
}

uint64_t CX_ConstantlySwappingThread::getFrameNumber (void) {
	uint64_t frameNumber = 0;
	if (lock()) {
		frameNumber = _frameCount;
		unlock();
	}
	return frameNumber;
}
