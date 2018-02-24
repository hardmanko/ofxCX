#pragma once

//#include <future>

//#include "CX_Clock.h"
#include "CX_DataFrameCell.h"
#include "CX_Private.h" // CX_GLFenceSync ???

namespace CX {

	class CX_Display;

namespace Private {

	void swapVideoBuffers(bool glFinish);

	class CX_DisplaySwapThread {
	public:

		struct BufferSwapData {
			CX_Millis time;
		};



		struct Configuration {

			Configuration(void) :
				display(nullptr),
				swapContinuously(false),
				glFinishAfterSwap(false),
				unlockMutexDuringSwap(true),
				sleepTimePerLoop(0.5),
				//preSwapCpuHogging(0),
				postSwapSleep(0)
			{}

			CX_Display* display;
			std::function<uint64_t(const BufferSwapData&)> bufferSwapCallback;

			bool swapContinuously;
			bool glFinishAfterSwap;
			bool unlockMutexDuringSwap;
			CX_Millis sleepTimePerLoop;
			//CX_Millis preSwapCpuHogging;
			CX_Millis postSwapSleep;
		};

		CX_DisplaySwapThread(void);

		bool setup(Configuration config, bool startThread);
		Configuration getConfiguration(void);


		void setSwapContinuously(bool swapContinuously);
		bool getSwapContinuously(void);

		void setGLFinishAfterSwap(bool glFinishAfterSwap);
		void setSleepTimePerLoop(CX_Millis sleepTime);
		//void setPreSwapCpuHogging(CX_Millis preSwapHogging);
		void setUnlockMutexDuringSwap(bool unlock);
		void setPostSwapSleep(CX_Millis sleep);
		//void setSleepProportionOfSwapInterval(double sleepProportion);

		//void setFrameNumber(uint64_t frameNumber);

		void start(void);
		void stop(bool wait);
		bool isRunning(void);


		// Queued mode
		bool queueSwaps(unsigned int n);
		unsigned int queuedSwapCount(void);
		void clearQueuedSwaps(void);



		
		// Queued frames
		struct QueuedFrameResult {
			bool renderTimeValid;
			CX_Millis renderCompleteTime;

			CX_Millis startTime;
			uint64_t startFrame;
		};

		struct QueuedFrameConfig {
			bool acquireRenderingContext;
			CX_Display* display;
			//std::function<void(const QueuedFrameResult&)> frameCompleteCallback;
		};
		bool configureQueuedFrameMode(QueuedFrameConfig config);

		

		struct QueuedFrame {
			uint64_t startFrame;

			std::shared_ptr<ofFbo> fbo;
			std::function<void(CX_Display&)> fun;

			std::function<void(const QueuedFrameResult&)> frameCompleteCallback;
		};

		bool queueFrame(std::shared_ptr<QueuedFrame> qf);
		bool requeueFrame(uint64_t oldFrame, uint64_t newFrame);
		bool requeueAllFrames(int64_t offset);

		unsigned int getQueuedFrameCount(void);
		void clearQueuedFrames(void);


		// Commands
		enum class CommandType : int {
			SetSwapInterval, // "swapInterval": unsigned int (0 or 1)
			QueueSwaps, // "swaps": unsigned int
			//RequeueFrame, // "oldFrame": uint64_t, "newFrame": uint64_t
			//DeleteQueuedFrame, // "frame": unsigned int
			//DeleteAllQueuedFrames, // void
			ExecuteFunction, // use fun()
			AcquireRenderingContext // "acquire": bool
		};
		struct CommandResult;
		struct Command {
			CommandType type;
			std::map<std::string, CX_DataFrameCell> values;
			std::function<void(void)> fun;
			std::function<void(CommandResult&&)> callback;
		};
		enum class CommandCode : int {
			Failure,
			Success,
			Unimplemented
		};
		struct CommandResult {
			Command command;
			CommandCode code;
		};
		struct CommandConfig {
			CommandConfig(void) :
				sleepUnit(CX_Micros(100))
			{}
			CX_Millis sleepUnit;
			//std::function<void(CommandResult&&)> callback;
		};
		void configureCommands(CommandConfig config);
		bool queueCommand(std::shared_ptr<Command> cmd, bool wait);
		bool commandQueueSwaps(unsigned int swaps, bool wait, std::function<void(CommandResult&&)> callback = nullptr);
		bool commandSetSwapInterval(unsigned int swapInterval, bool wait, std::function<void(CommandResult&&)> callback = nullptr);
		bool commandExecuteFunction(std::function<void(void)> fun, bool wait, std::function<void(CommandResult&&)> callback = nullptr);
		
		bool commandAcquireRenderingContext(bool acquire, bool wait, std::function<void(CommandResult&&)> callback = nullptr);


		bool threadOwnsRenderingContext(void);

	private:

		std::recursive_mutex _mutex;

		uint64_t _threadFrameNumber;

		bool _threadRunning;
		bool _threadOwnsRenderingContext;
		std::thread _thread;

		unsigned int _queuedSwaps;

		Configuration _config;

		void _threadFunction(void);
		void _swap(void);


		QueuedFrameConfig _qfConfig; // under _mutex
		std::recursive_mutex _queuedFramesMutex;
		std::map<uint64_t, std::shared_ptr<QueuedFrame>> _queuedFrames;
		struct {
			std::shared_ptr<QueuedFrame> frame;
			CX_GLFenceSync fenceSync;
		} _currentQF;

		void _queuedFrameTask(void);
		void _drawQueuedFrameIfNeeded(void);

		void _queuedFramePostSwapTask(uint64_t swapFrame, CX_Millis swapTime);
		

		// Commands
		CommandConfig _cmdConfig; // under _mutex
		std::recursive_mutex _commandQueueMutex;
		std::deque<std::shared_ptr<Command>> _commandQueue;
		void _processQueuedCommands(void);

		CommandCode _acquireRenderingContext(bool acquire);
	};



}
}