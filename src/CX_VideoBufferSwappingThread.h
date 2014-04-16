#pragma once

#include <deque>

#include "ofThread.h"

#include "CX_Clock.h"

namespace CX {
namespace Private {

	class CX_VideoBufferSwappingThread : public ofThread {
	public:

		CX_VideoBufferSwappingThread(void);

		void threadedFunction(void) override;

		void swapNFrames(unsigned int n);

		bool hasSwappedSinceLastCheck(void);

		//void pauseSwapping(void);
		//void unpauseSwapping(void);

		CX_Millis getTypicalSwapPeriod(void);
		CX_Millis getLastSwapTime(void);
		CX_Millis getLastSwapPeriod(void);
		CX_Millis estimateNextSwapTime(void);
		uint64_t getFrameNumber(void);

		void setGLFinishAfterSwap(bool finishAfterSwap);

	private:

		bool _lockMutex(void);
		bool _unlockMutex(void);
		bool _isLocked;

		deque<CX_Millis> _recentSwapTimes;

		uint64_t _frameCount;
		uint64_t _frameCountOnLastCheck;

		int _swapsBeforeStop;

		bool _glFinishAfterSwap;

		//bool _swappingPaused;
	};

}
}