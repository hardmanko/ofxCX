#ifndef _CX_SWAPPING_THREAD_H_
#define _CX_SWAPPING_THREAD_H_

#include <deque>

#include "ofThread.h"

#include "CX_Clock.h"

namespace CX {

	class CX_ConstantlySwappingThread : public ofThread {
	public:

		CX_ConstantlySwappingThread (void);

		void threadedFunction (void);

		void swapNFrames (unsigned int n);

		bool swappedSinceLastCheck (void);

		CX_Micros getTypicalSwapPeriod (void);
		CX_Micros getLastSwapTime (void);
		CX_Micros getLastSwapPeriod (void);
		CX_Micros estimateNextSwapTime (void);
		uint64_t getFrameNumber (void);

	private:

		bool _lockMutex (void);
		bool _unlockMutex (void);
		bool _isLocked;
		//uint64_t _lastSwapTime;

		//uint64_t _framePeriod;
		//deque<uint64_t> _recentSwapPeriods;
		deque<CX_Micros> _recentSwapTimes;

		uint64_t _frameCount;
		uint64_t _frameCountOnLastCheck;

		int _swapsBeforeStop;

	};

}

#endif //_CX_SWAPPING_THREAD_H_