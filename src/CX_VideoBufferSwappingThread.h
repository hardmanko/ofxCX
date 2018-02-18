#pragma once

#include <future>

#include "ofThread.h"

#include "CX_Clock.h"
#include "CX_Algorithm.h"

namespace CX {
namespace Private {

	class CX_VideoBufferThread {
	public:

		struct Configuration {

			Configuration(void) :
				swapContinuously(false),
				glFinishAfterSwap(false),
				unlockMutexDuringSwap(true),
				sleepTimePerLoop(0),
				preSwapCpuHogging(0)
			{}

			bool swapContinuously;
			bool glFinishAfterSwap;
			bool unlockMutexDuringSwap;
			CX_Millis sleepTimePerLoop;
			CX_Millis preSwapCpuHogging;
		};

		void setup(Configuration config, bool startThread);
		const Configuration& getConfiguration(void);

		void setSwapContinuously(bool swapContinuously);
		void setGLFinishAfterSwap(bool glFinishAfterSwap);
		void setSleepTimePerLoop(CX_Millis sleepTime);
		void setPreSwapCpuHogging(CX_Millis preSwapHogging);
		void setUnlockMutexDuringSwap(bool unlock);


		void start(void);
		void stop(bool wait);
		bool isRunning(void);


		void queueSwaps(unsigned int n);


		CX_Millis getLastSwapTime(void);
		CX_Millis estimateNextSwapTime(void);
		CX_Millis estimateLaggedSwapTime(int lag);

		
		int64_t getFrameNumber(void);

		// Synchronization
		bool hasSwappedSinceLastCheck(bool reset = true);
		int64_t swapsSinceLastCheck(bool reset = true);

		struct VideoSwapData {
			CX_Millis time;
			int64_t frameCount;
		};

		ofEvent<const VideoSwapData&> swapEvent;

		// This isn't really synchronization. It's more making sure the frame period is stable
		struct SynchronizeConfig {

			SynchronizeConfig(void) :
				requiredSwaps(3),
				nominalFramePeriod(-100),
				periodTolerance(3),
				startThreadIfStopped(true),
				stopSynchronizingOnceSynchronized(false)
			{}

			unsigned int requiredSwaps;
			CX_Millis nominalFramePeriod;
			CX_Millis periodTolerance;

			bool startThreadIfStopped;
			bool stopSynchronizingOnceSynchronized;

		};

		bool startSynchronizing(SynchronizeConfig config);
		bool isSynchronized(bool stopIfSynchronized);
		bool isSynchronizing(void);
		
		void stopSynchronizing(void);
		bool synchronize(SynchronizeConfig config, CX_Millis timeout);

		void clearSwapHistory(void);



	private:

		std::recursive_mutex _mutex;


		bool _threadRunning;
		std::thread _thread;

		unsigned int _queuedSwaps;

		CX_Millis _lastSwapTime;

		int64_t _frameCount;
		int64_t _frameCountOnLastCheck;

		Algo::RollingLinearModel _swapLM;

		
		Configuration _config;

		void _threadFunction(void);
		void _swap(void);

		bool _synchronizing;
		bool _previous_swapContinuously;
		SynchronizeConfig _syncConfig;
		std::deque<CX_Millis> _syncData;

		
	};

}
}