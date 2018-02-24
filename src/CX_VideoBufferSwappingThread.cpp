#include "CX_VideoBufferSwappingThread.h"

#include "CX_Display.h"
#include "CX_Private.h" //glfwContext

namespace CX {
namespace Private {

void swapVideoBuffers(bool glFinishAfterSwap) {
	glfwSwapBuffers(CX::Private::glfwContext);
	if (glFinishAfterSwap) {
		glFinish();
	}
}

CX_DisplaySwapThread::CX_DisplaySwapThread(void) :
	_threadFrameNumber(0),
	_threadRunning(false),
	_queuedSwaps(0),
	_threadOwnsRenderingContext(false)
{}

bool CX_DisplaySwapThread::setup(Configuration config, bool startThread) {

	std::lock_guard<std::recursive_mutex> lock(_mutex);

	if (config.bufferSwapCallback == nullptr) {
		return false;
	}

	if (_threadRunning) {
		stop(true);
	}

	_config = config;

	if (startThread) {
		start();
	}

	return true;
}

// It would be unsafe to return a reference. You should have getters for each setting.
CX_DisplaySwapThread::Configuration CX_DisplaySwapThread::getConfiguration(void) {
	return _config;
}

void CX_DisplaySwapThread::setSwapContinuously(bool swapContinuously) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_config.swapContinuously = swapContinuously;
	if (swapContinuously) {
		_queuedSwaps = 0;
	}
}

bool CX_DisplaySwapThread::getSwapContinuously(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _config.swapContinuously;
}

void CX_DisplaySwapThread::setGLFinishAfterSwap(bool glFinishAfterSwap) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_config.glFinishAfterSwap = glFinishAfterSwap;
}

void CX_DisplaySwapThread::setSleepTimePerLoop(CX_Millis sleepTime) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_config.sleepTimePerLoop = sleepTime;
}

/*
void CX_DisplaySwapThread::setPreSwapCpuHogging(CX_Millis preSwapHogging) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_config.preSwapCpuHogging = preSwapHogging;
}
*/

void CX_DisplaySwapThread::setUnlockMutexDuringSwap(bool unlock) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_config.unlockMutexDuringSwap = unlock;
}

void CX_DisplaySwapThread::setPostSwapSleep(CX_Millis sleep) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_config.postSwapSleep = sleep;
}

/*
void CX_DisplaySwapThread::setFrameNumber(uint64_t frameNumber) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_threadFrameNumber = frameNumber;
}
*/

void CX_DisplaySwapThread::start(void) {

	std::lock_guard<std::recursive_mutex> lock(_mutex);

	if (_threadRunning) {
		return;
	}

	_queuedSwaps = 0;

	_threadRunning = true;
	_thread = std::thread(&CX_DisplaySwapThread::_threadFunction, this);
	
}

void CX_DisplaySwapThread::stop(bool wait) {
	_mutex.lock();

	if (!_threadRunning) {
		_mutex.unlock();
		return;
	}

	_acquireRenderingContext(false); // release rendering context if stopping thread

	_threadRunning = false;

	_mutex.unlock(); // Essential unlock before waiting on thread, which must check state of _threadRunning in order to exit.

	if (wait) {
		_thread.join();
	}
}

bool CX_DisplaySwapThread::isRunning(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _threadRunning;
}


bool CX_DisplaySwapThread::queueSwaps(unsigned int n) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	if (_config.swapContinuously) {
		return false;
	}

	_queuedSwaps += n;

	return true;
}

unsigned int CX_DisplaySwapThread::queuedSwapCount(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _queuedSwaps;
}

void CX_DisplaySwapThread::_threadFunction(void) {

	while (this->isRunning()) {

		_queuedFrameTask();

		_processQueuedCommands();

		_mutex.lock();
		bool shouldSwap = _config.swapContinuously || _queuedSwaps > 0;
		CX_Millis sleepTime = _config.sleepTimePerLoop;
		_mutex.unlock();

		if (shouldSwap) {
			_swap();
			sleepTime = _config.postSwapSleep;
		}

		// TODO: Sleep in little segments whlie doing processing?
		Instances::Clock.sleep(sleepTime);

	}

}

void CX_DisplaySwapThread::_swap(void) {

	_mutex.lock();

	bool glFinishAfterSwap = _config.glFinishAfterSwap;
	bool unlockMutexDuringSwap = _config.unlockMutexDuringSwap;

	if (unlockMutexDuringSwap) {
		_mutex.unlock();
	}

	Private::swapVideoBuffers(glFinishAfterSwap);

	CX_Millis swapTime = CX::Instances::Clock.now();

	if (unlockMutexDuringSwap) {
		_mutex.lock();
	}

	if (_config.bufferSwapCallback != nullptr) {
		BufferSwapData bsd{ swapTime };
		_threadFrameNumber = _config.bufferSwapCallback(bsd);
	}

	if (_queuedSwaps > 0) {
		_queuedSwaps--;
	}

	_queuedFramePostSwapTask(_threadFrameNumber, swapTime);

	_mutex.unlock();

}

CX_DisplaySwapThread::CommandCode CX_DisplaySwapThread::_acquireRenderingContext(bool acquire) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	if (acquire == threadOwnsRenderingContext()) {
		return CommandCode::Success;
	}

	Private::CX_GlfwContextManager& cm = Private::glfwContextManager;

	if (acquire) {
		if (cm.trylock()) {
			_threadOwnsRenderingContext = true;
		} else {
			return CommandCode::Failure;
		}
	} else {
		if (cm.isLockedByThisThread()) {
			cm.unlock();
		}
		_threadOwnsRenderingContext = false;
	}

	return CommandCode::Success;
}


bool CX_DisplaySwapThread::threadOwnsRenderingContext(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _threadOwnsRenderingContext;
}

void CX_DisplaySwapThread::configureCommands(CommandConfig config) {
	_cmdConfig = config;
}

// always returns false if not waiting
bool CX_DisplaySwapThread::queueCommand(std::shared_ptr<Command> cmd, bool wait) {
	if (!wait) {
		std::lock_guard<std::recursive_mutex> lock(_commandQueueMutex);

		_commandQueue.push_back(cmd);

		return false;
	}

	// do stuff to wait
	bool signal = false;
	bool success = false;

	std::function<void(CommandResult&&)> userCallback = cmd->callback;

	auto wrapperCallback = [&](CommandResult&& cr) {
		signal = true;
		success = cr.code == CommandCode::Success;
		if (userCallback != nullptr) {
			userCallback(std::move(cr)); // callback still gets called
		}
	};

	cmd->callback = wrapperCallback;

	_commandQueueMutex.lock();
	_commandQueue.push_back(cmd);
	_commandQueueMutex.unlock();

	while (!signal) {
		Instances::Clock.sleep(CX_Micros(100)); // less? configurable?
	}

	return success;
}

bool CX_DisplaySwapThread::commandQueueSwaps(unsigned int swaps, bool wait, std::function<void(CommandResult&&)> callback) {
	std::shared_ptr<Command> cmd = std::make_shared<Command>();

	cmd->type = CommandType::QueueSwaps;
	cmd->values["swaps"] = swaps;
	cmd->callback = callback;

	return queueCommand(cmd, wait);
}

bool CX_DisplaySwapThread::commandSetSwapInterval(unsigned int swapInterval, bool wait, std::function<void(CommandResult&&)> callback) {
	std::shared_ptr<Command> cmd = std::make_shared<Command>();

	cmd->type = CommandType::SetSwapInterval;
	cmd->values["swapInterval"] = swapInterval;
	cmd->callback = callback;

	return queueCommand(cmd, wait);
}

bool CX_DisplaySwapThread::commandExecuteFunction(std::function<void(void)> fun, bool wait, std::function<void(CommandResult&&)> callback) {
	std::shared_ptr<Command> cmd = std::make_shared<Command>();

	cmd->type = CommandType::ExecuteFunction;
	cmd->fun = fun;
	cmd->callback = callback;

	return queueCommand(cmd, wait);
}

bool CX_DisplaySwapThread::commandAcquireRenderingContext(bool acquire, bool wait, std::function<void(CommandResult&&)> callback) {
	std::shared_ptr<Command> cmd = std::make_shared<Command>();

	cmd->type = CommandType::AcquireRenderingContext;
	cmd->values["acquire"] = acquire;
	cmd->callback = callback;

	return queueCommand(cmd, wait);
}

void CX_DisplaySwapThread::_processQueuedCommands(void) {

	std::lock_guard<std::recursive_mutex> lock(_commandQueueMutex);

	for (std::shared_ptr<Command>& cmd : _commandQueue) {

		CommandCode code = CommandCode::Failure;

		switch (cmd->type) {
		case CommandType::QueueSwaps:
			code = CommandCode::Unimplemented;
			break;
		case CommandType::SetSwapInterval:
			code = CommandCode::Unimplemented;
			break;
		case CommandType::AcquireRenderingContext:
		{
			bool acquire = cmd->values["acquire"];
			code = _acquireRenderingContext(acquire);
		}
			break;
		case CommandType::ExecuteFunction:
			//CX_DataFrameCell funRes = cmd->fun();
			break;
		}

		if (cmd->callback != nullptr) {
			auto cbCopy = cmd->callback;
			CommandResult result{ std::move(*cmd), code };

			cbCopy(std::move(result));
		}

	}

	_commandQueue.clear();

}

bool CX_DisplaySwapThread::configureQueuedFrameMode(QueuedFrameConfig config) {
	_qfConfig = config;
	return true;
}

bool CX_DisplaySwapThread::queueFrame(std::shared_ptr<QueuedFrame> qf) {

	if (qf->fbo == nullptr && qf->fun == nullptr) {
		// fail: not set up. This should be impossible with a good interface
		return false;
	}

	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		if (qf->startFrame <= _threadFrameNumber) {
			Instances::Log.warning("CX_DisplaySwapThread") << "Queued frame for frame number " << qf->startFrame <<
				" arrived late (on frame number " << _threadFrameNumber << ") and was ignored.";
			return false;
		}
	}

	std::lock_guard<std::recursive_mutex> qfLock(_queuedFramesMutex);

	if (_queuedFrames.find(qf->startFrame) != _queuedFrames.end()) {
		Instances::Log.notice("CX_DisplaySwapThread") << "Queued frame for frame number " << qf->startFrame << " replaced.";
	}

	_queuedFrames[qf->startFrame] = qf;

	return true;
}

bool CX_DisplaySwapThread::requeueFrame(uint64_t oldFrame, uint64_t newFrame) {
	std::lock_guard<std::recursive_mutex> qfLock(_queuedFramesMutex);

	if (_queuedFrames.find(oldFrame) == _queuedFrames.end()) {
		Instances::Log.warning("CX_DisplaySwapThread") << "requeueFrame(): Nothing queued for frame " << oldFrame << ".";
		return false;
	}

	if (_queuedFrames.find(newFrame) != _queuedFrames.end()) {
		Instances::Log.warning("CX_DisplaySwapThread") << "requeueFrame(): Frame queued for frame " << 
			newFrame << " was replaced with the frame queued for frame " << oldFrame << ".";
	}

	_queuedFrames[newFrame] = _queuedFrames[oldFrame];
	_queuedFrames.erase(oldFrame);

	return true;
}

bool CX_DisplaySwapThread::requeueAllFrames(int64_t offset) {


	return false;
}

unsigned int CX_DisplaySwapThread::getQueuedFrameCount(void) {
	std::lock_guard<std::recursive_mutex> qfLock(_queuedFramesMutex);
	return _queuedFrames.size();
}

void CX_DisplaySwapThread::clearQueuedFrames(void) {
	std::lock_guard<std::recursive_mutex> qfLock(_queuedFramesMutex);
	_queuedFrames.clear();
}

void CX_DisplaySwapThread::_queuedFrameTask(void) {
	_drawQueuedFrameIfNeeded();

	std::lock_guard<std::recursive_mutex> qfLock(_queuedFramesMutex);
	if (_currentQF.frame != nullptr && _currentQF.fenceSync.isSyncing()) {
		_currentQF.fenceSync.updateSync();
	}
}

void CX_DisplaySwapThread::_queuedFramePostSwapTask(uint64_t swapFrame, CX_Millis swapTime) {

	std::lock_guard<std::recursive_mutex> qfLock(_queuedFramesMutex);

	if (_currentQF.frame == nullptr) {
		return;
	}

	if (_currentQF.frame->frameCompleteCallback != nullptr) {

		QueuedFrameResult result;

		result.startFrame = swapFrame;
		result.startTime = swapTime;

		result.renderTimeValid = _currentQF.fenceSync.isSynced();

		if (_currentQF.fenceSync.isSynced()) {
			result.renderCompleteTime = _currentQF.fenceSync.getSyncTime();
		}

		_currentQF.frame->frameCompleteCallback(result);

	}

	_currentQF.frame = nullptr;

	_drawQueuedFrameIfNeeded();
}



void CX_DisplaySwapThread::_drawQueuedFrameIfNeeded(void) {

	std::unique_lock<std::recursive_mutex> lock(_mutex, std::defer_lock);
	std::unique_lock<std::recursive_mutex> qfLock(_queuedFramesMutex, std::defer_lock);
	std::lock(lock, qfLock);


	auto thisIt = _queuedFrames.find(_threadFrameNumber);
	if (thisIt == _queuedFrames.end()) {
		// not found
		return;
	}

	_currentQF.frame = thisIt->second; // copy the shared_ptr
	
	// Delete any frames as old as this frame or older
	_queuedFrames.erase(_queuedFrames.begin(), thisIt);

	std::shared_ptr<QueuedFrame>& qf = _currentQF.frame; // shorthand



	if (!Private::glfwContextManager.isLockedByThisThread()) {
		// Error: Context is unavailable
		return;
	}

	_config.display->beginDrawingToBackBuffer();

	if (qf->fbo != nullptr) {
		ofPushStyle();
		ofDisableAlphaBlending();
		ofSetColor(255);
		qf->fbo->draw(0, 0);
		ofPopStyle();
	} else if (qf->fun != nullptr) {
		qf->fun(*_config.display);
	}

	_config.display->endDrawingToBackBuffer();

	_currentQF.fenceSync.startSync();
}

// For reference, some good ideas:
/*
void CX_SlidePresenter::_renderCurrentSlide(void) {

	_config.display->beginDrawingToBackBuffer();
	if (_slides.at(_currentSlide).drawingFunction != nullptr) {
		_slides.at(_currentSlide).drawingFunction();
	} else {
		ofPushStyle();
		ofDisableAlphaBlending();
		ofSetColor(255);
		_slides.at(_currentSlide).framebuffer.draw(0, 0);
		ofPopStyle();
	}
	_config.display->endDrawingToBackBuffer();

	CX::Instances::Log.verbose("CX_SlidePresenter") << "Slide #" << _currentSlide << " rendering started at " << CX::Instances::Clock.now();

	if (_config.useFenceSync) {
		_slideInfo.at(_currentSlide).fenceSyncObject = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		glFlush(); //This glFlush assures that the fence sync object gets pushed into the command queue.
		_slideInfo.at(_currentSlide).awaitingFenceSync = true;

		_slides.at(_currentSlide).presentationStatus = Slide::PresStatus::RENDERING;
	} else {
		_slides.at(_currentSlide).presentationStatus = Slide::PresStatus::SWAP_PENDING;
	}
}
*/

} //namespace Private
} //namespace CX