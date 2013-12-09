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

		bool swappedSinceLastCheck (void);

		uint64_t getTypicalSwapPeriod (void);
		uint64_t getLastSwapTime (void);
		uint64_t getLastSwapPeriod (void);
		uint64_t estimateNextSwapTime (void);
		uint64_t getFrameNumber (void);

	private:

		uint64_t _lastSwapTime;

		//uint64_t _framePeriod;
		deque<uint64_t> _recentSwapPeriods;

		uint64_t _frameCount;
		uint64_t _frameCountOnLastCheck;

	};

}

#endif //_CX_SWAPPING_THREAD_H_