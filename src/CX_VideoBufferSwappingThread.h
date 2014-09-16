#pragma once

#include "ofThread.h"

#include "CX_Clock.h"

namespace CX {
namespace Private {

	class CX_VideoBufferSwappingThread : public ofThread {
	public:

		CX_VideoBufferSwappingThread(void);

		void threadedFunction(void) override;

		void swapNFrames(int n);
		bool hasSwappedSinceLastCheck(void);
		CX_Millis getLastSwapTime(void);
		uint64_t getFrameNumber(void);

		void setGLFinishAfterSwap(bool finishAfterSwap);

	private:

		bool _lockMutex(void);
		bool _unlockMutex(void);
		bool _isLocked;

		CX_Millis _lastSwapTime;

		uint64_t _frameCount;
		uint64_t _frameCountOnLastCheck;

		int _swapsBeforeStop;

		bool _glFinishAfterSwap;

	};

}
}