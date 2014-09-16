#include "CX_VideoBufferSwappingThread.h"

#include "CX_Private.h" //glfwContext

namespace CX {
namespace Private {

CX_VideoBufferSwappingThread::CX_VideoBufferSwappingThread(void) :
	_isLocked(false),
	_lastSwapTime(0),
	_frameCount(0),
	_frameCountOnLastCheck(0),
	_swapsBeforeStop(-1),
	_glFinishAfterSwap(false)
{
}

void CX_VideoBufferSwappingThread::threadedFunction(void) {

	while (isThreadRunning()) {

		glfwSwapBuffers(CX::Private::glfwContext);

		if (lock()) {
			bool finish = _glFinishAfterSwap;
			unlock();
			if (finish) {
				glFinish();
			}
		}

		CX_Millis swapTime = CX::Instances::Clock.now();

		if (lock()) {

			++_frameCount;

			_lastSwapTime = swapTime;

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
	}
}


void CX_VideoBufferSwappingThread::swapNFrames(int n) {
	if (n <= 0) {
		return;
	}

	if (!this->isThreadRunning()) {
		_swapsBeforeStop = n;
		this->startThread(true);
	} else {
		if (_lockMutex()) {
			_swapsBeforeStop = n;
			_unlockMutex();
		}
	}
}

bool CX_VideoBufferSwappingThread::hasSwappedSinceLastCheck(void) {
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

CX_Millis CX_VideoBufferSwappingThread::getLastSwapTime(void) {
	CX_Millis lastSwapTime = 0;
	if (_lockMutex()) {
		lastSwapTime = _lastSwapTime;
		_unlockMutex();
	}
	return lastSwapTime;
}

uint64_t CX_VideoBufferSwappingThread::getFrameNumber(void) {
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


bool CX_VideoBufferSwappingThread::_lockMutex(void) {
	if (_isLocked) {
		return true;
	}

	if (lock()) {
		_isLocked = true;
		return true;
	}
	return false;
}

bool CX_VideoBufferSwappingThread::_unlockMutex(void) {
	_isLocked = false;
	unlock();
	return true;
}


} //namespace Private
} //namespace CX