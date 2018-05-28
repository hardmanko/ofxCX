#pragma once

#include "CX_Definitions.h"
#include "CX_DataFrameCell.h"
#include "CX_Private.h" // CX_GLFenceSync ???
#include "CX_SynchronizationUtils.h"
#include "CX_DisplaySwapper.h"

namespace CX {

	//class CX_CLASS(Display);
	class CX_Display;

	namespace Private {
		void swapVideoBuffers(bool glFinish);
	}

	class CX_DisplayThread {
	public:

		struct Configuration {

			CX_Millis preSwapSafetyBuffer;

			//CX_Millis requiredSwapDuration; // duration of swaps required for data and swap lock

			//CX_Millis nominalFramePeriod;
			//double framePeriodTolerance;

			bool enableFrameQueue;

		};

		~CX_DisplayThread(void);

		// I don't think that anything uses these functions, so they can probably be removed
		bool tryLock(std::string lockOwner);
		bool isLocked(void);
		std::string getLockOwner(void);
		void unlock(void);
		

		bool setup(Configuration config, bool startThread);
		Configuration getConfiguration(void);


		void startThread(void);
		void stopThread(bool wait = true);
		bool isThreadRunning(void);

		ofEvent<void> updateEvent;
		// for the swap event, use Disp.swapData.newDataEvent

		// do not depend on swap lock
		//CX_Millis getLastSwapTime(void); // don't need or want these functions, right? use display functions
		//FrameNumber getFrameNumber(void);
		//bool hasSwappedSinceLastCheck(void); // don't need or want these functions, right? use display functions
		//void waitForSwap(void);



		///////////////
		// Swap Lock //
		///////////////

		// don't need/move to display
		bool isSwappingStably(void); // dataUser
		bool waitForStableSwapping(CX_Millis timeout); // dataUser

		///////////////////
		// Queued frames //
		///////////////////

		struct QueuedFrameResult {
			FrameNumber desiredStartFrame;
			FrameNumber actualStartFrame;

			CX_Millis startTime;

			bool renderTimeValid;
			CX_Millis renderCompleteTime;
		};
		
		struct QueuedFrame {
			FrameNumber startFrame;

			std::shared_ptr<ofFbo> fbo; // make not pointer?
			std::function<void(void)> fun;

			std::function<void(QueuedFrameResult&&)> frameCompleteCallback;
		};

		bool enableFrameQueue(bool enable);

		// These functions do the same thing
		bool frameQueueEnabled(void);
		bool threadOwnsRenderingContext(void);
		
		bool queueFrame(std::shared_ptr<QueuedFrame> qf);
		bool queueFrame(FrameNumber startFrame, std::function<void(void)> fun, std::function<void(QueuedFrameResult&&)> frameCompleteCallback = nullptr);
		bool queueFrame(FrameNumber startFrame, std::shared_ptr<ofFbo> fbo, std::function<void(QueuedFrameResult&&)> frameCompleteCallback = nullptr);

		//bool deleteFrame()
		bool requeueFrame(FrameNumber oldFrame, FrameNumber newFrame);
		bool requeueAllFrames(int offset);
		void clearQueuedFrames(void);
		//std::vector<uint64_t> getQueuedFrameNumbers(void);

		std::shared_ptr<QueuedFrame> getQueuedFrame(unsigned int index); // doesn't this have too high of a power level?
		unsigned int getQueuedFrameCount(void);


		

	private:

		friend class CX::CX_Display;

		CX_DisplayThread(CX_Display* disp, std::function<void(CX_Display*)> swapFun);
		
		CX_Display* _display;
		std::function<void(void)> _bufferSwapFunction;

		CX_DisplaySwapper _displaySwapper;

		std::recursive_mutex _mutex;
		
		std::string _lockOwner;

		Configuration _config;

		bool _threadRunning;
		bool _hasSwappedSinceLastCheck;

		std::thread _thread;


		void _threadFunction(void);
		void _swap(void);


		

		// Queued frames
		std::recursive_mutex _queuedFramesMutex;
		std::deque<std::shared_ptr<QueuedFrame>> _queuedFrames;
		struct {
			std::shared_ptr<QueuedFrame> frame;
			Private::CX_GLFenceSync fenceSync;
		} _currentQF;

		void _queuedFrameTask(void);
		void _drawQueuedFrameIfNeeded(void);

		void _queuedFramePostSwapTask(void);
		
		

		//////////////
		// Commands //
		//////////////

		enum class CommandType : int {
			SetSwapInterval, // "swapInterval": unsigned int (0 or 1)
			AcquireRenderingContext, // "acquire": bool
			//SetGLFinishAfterSwap, // "finish": bool
			//SetSwapPeriod, // "period": CX_Millis
			ExecuteFunction // uses fun()
		};
		struct CommandResult;
		struct Command {
			CommandType type;
			std::map<std::string, CX_DataFrameCell> values;
			std::function<bool(void)> fun;
			std::function<void(CommandResult&&)> callback;
		};
		struct CommandResult {
			Command command;
			bool success;
		};

		// command functions may not be called with a wait while the mutex is locked
		bool commandSetSwapInterval(unsigned int swapInterval, bool wait = true, std::function<void(CommandResult&&)> callback = nullptr);
		bool commandAcquireRenderingContext(bool acquire, bool wait = true, std::function<void(CommandResult&&)> callback = nullptr);
		//bool commandSetGLFinishAfterSwap(bool finish, bool wait = true, std::function<void(CommandResult&&)> callback = nullptr);
		//bool commandSetSwapPeriod(CX_Millis period, bool wait = true, std::function<void(CommandResult&&)> callback = nullptr);
		bool commandExecuteFunction(std::function<bool(void)> fun, bool wait = true, std::function<void(CommandResult&&)> callback = nullptr);

		std::recursive_mutex _commandQueueMutex;
		std::deque<std::shared_ptr<Command>> _commandQueue;
		bool _queueCommand(std::shared_ptr<Command> cmd, bool wait);
		void _processQueuedCommands(void);

		bool _acquireRenderingContext(bool acquire);
	};

	class CX_SlideQueue {
		// Move the queued frames logic into this class.
		// The exception is that enableFrameQueue() might stay with CX_DisplayThread, but be renamed renderOnDisplayThread().
		// Use CX_SlideBuffer::Slide.
	};


} // namespace CX