#include "CX_DisplayThread.h"

#include "CX_Display.h"
#include "CX_Private.h"

namespace CX {
namespace Private {

	void swapVideoBuffers(bool glFinishAfterSwap) {
		glfwSwapBuffers(CX::Private::glfwContext);
		if (glFinishAfterSwap) {
			glFinish();
		}
	}

} // namespace Private

CX_DisplayThread::~CX_DisplayThread(void) {
	stopThread(true);
}

CX_DisplayThread::CX_DisplayThread(CX_Display* disp, std::function<void(CX_Display*)> swapFun) :
	_threadRunning(false),
	_hasSwappedSinceLastCheck(false),
	_display(disp)
{
	_bufferSwapFunction = std::bind(swapFun, disp);
}


bool CX_DisplayThread::setup(const Configuration& config, bool startThread) {

	if (isThreadRunning()) {
		stopThread(true);
	}

	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);

		
		CX_DisplaySwapper::Configuration dsc;
		dsc.display = _display;
		dsc.client = &_display->swapClient;
		dsc.preSwapSafetyBuffer = config.preSwapSafetyBuffer;
		dsc.mode = CX_DisplaySwapper::Mode::Prediction;

		if (!_displaySwapper.setup(dsc)) {
			return false;
		}

		_config = config;

		if (startThread) {
			this->startThread();
		}
	}

	if (isThreadRunning()) {
		this->enableFrameQueue(config.enableFrameQueue);
	}

	return true;
}

// It would be unsafe to return a reference. You should have getters for each setting.
const CX_DisplayThread::Configuration& CX_DisplayThread::getConfiguration(void) {
	return _config;
}




void CX_DisplayThread::startThread(void) {

	std::lock_guard<std::recursive_mutex> lock(_mutex);
	//_mutex.lock();

	if (_threadRunning) {
		return;
	}

	_threadRunning = true;
	_thread = std::thread(&CX_DisplayThread::_threadFunction, this);
	
	//_mutex.unlock();

	//Instances::Log.notice() << "startThread(): Thread started at " << Instances::Clock.now().seconds();

	//while (!hasSwappedSinceLastCheck()) {
	//	Instances::Log.notice() << "waiting for first swap";
	//}
}

void CX_DisplayThread::stopThread(bool wait) {
	

	if (!isThreadRunning()) {
		return;
	}

	enableFrameQueue(false); // release rendering context if stopping thread
	//_acquireRenderingContext(false); 

	_mutex.lock();
	_threadRunning = false;
	_mutex.unlock(); // Essential unlock before waiting on thread, which must check state of _threadRunning in order to exit.

	if (wait) {
		_thread.join();
	}
}

bool CX_DisplayThread::isThreadRunning(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _threadRunning;
}


bool CX_DisplayThread::isSwappingStably(void) {
	// don't need to lock mutex because _display cannot change
	return this->_display->swapClient.verifier.isSwappingStably();
}

bool CX_DisplayThread::waitForStableSwapping(CX_Millis timeout) {
	return this->_display->swapClient.verifier.waitForStableSwapping(timeout);
}


void CX_DisplayThread::_threadFunction(void) {

	//Instances::Log.notice() << "startThread(): Thread function started at " << Instances::Clock.now().seconds();

	
	_mutex.lock(); // --- LOCK

	while (this->_threadRunning) {

		_queuedFrameTask();

		_processQueuedCommands();

		//CX_Millis preSwapSafetyBuffer = _config.preSwapSafetyBuffer;

		_mutex.unlock(); // --- UNLOCK
		
		ofNotifyEvent(updateEvent);

		if (_displaySwapper.shouldSwap()) {
			_swap();
		} else {
			std::this_thread::yield();
		}
		
		/*
		Sync::TimePrediction tp = _display->swapClient.predictTimeToNextSwap();
		if (tp.lowerBound() <= preSwapSafetyBuffer) {
			_swap();
		} else {
			std::this_thread::yield();
		}
		*/
		
		_mutex.lock(); // --- LOCK
		
	}

	_mutex.unlock(); // --- UNLOCK

	// failsafe
	if (Private::glfwContextManager.isLockedByThisThread()) {
		Instances::Log.warning("CX_DisplayThread") << "The rendering context was not already unlocked on thread exit. It was unlocked.";
		Private::glfwContextManager.unlock();
	}
}


void CX_DisplayThread::_swap(void) {

	_bufferSwapFunction();

	_mutex.lock();

	_hasSwappedSinceLastCheck = true;

	_queuedFramePostSwapTask();

	_mutex.unlock();

}


// always returns false if not waiting
bool CX_DisplayThread::_queueCommand(std::shared_ptr<Command> cmd, bool wait) {
	if (!isThreadRunning()) {
		Instances::Log.error("CX_DisplayThread") << "Command queued while thread was not running. It was ignored.";
		return false;
	}

	if (!wait) {
		// If not waiting, push the command into the queue for the thread to process
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
		success = cr.success;
		if (userCallback != nullptr) {
			userCallback(std::move(cr)); // callback still gets called
		}
	};

	cmd->callback = wrapperCallback;

	_commandQueueMutex.lock();
	_commandQueue.push_back(cmd);
	_commandQueueMutex.unlock();

	while (!signal) {
		std::this_thread::yield();
	}

	return success;
}


bool CX_DisplayThread::commandSetSwapInterval(unsigned int swapInterval, bool wait, std::function<void(CommandResult&&)> callback) {
	std::shared_ptr<Command> cmd = std::make_shared<Command>();

	cmd->type = CommandType::SetSwapInterval;
	cmd->values["swapInterval"] = swapInterval;
	cmd->callback = callback;

	return _queueCommand(cmd, wait);
}

/*
bool CX_DisplayThread::commandSetGLFinishAfterSwap(bool finish, bool wait, std::function<void(CommandResult&&)> callback) {
	std::shared_ptr<Command> cmd = std::make_shared<Command>();

	cmd->type = CommandType::SetGLFinishAfterSwap;
	cmd->values["finish"] = finish;
	cmd->callback = callback;

	return _queueCommand(cmd, wait);
}


bool CX_DisplayThread::commandSetSwapPeriod(CX_Millis period, bool wait, std::function<void(CommandResult&&)> callback) {
	std::shared_ptr<Command> cmd = std::make_shared<Command>();

	cmd->type = CommandType::SetSwapPeriod;
	cmd->values["period"] = period;
	cmd->callback = callback;

	return _queueCommand(cmd, wait);
}
*/

bool CX_DisplayThread::commandAcquireRenderingContext(bool acquire, bool wait, std::function<void(CommandResult&&)> callback) {
	std::shared_ptr<Command> cmd = std::make_shared<Command>();

	cmd->type = CommandType::AcquireRenderingContext;
	cmd->values["acquire"] = acquire;
	cmd->callback = callback;

	return _queueCommand(cmd, wait);
}

bool CX_DisplayThread::commandExecuteFunction(std::function<bool(void)> fun, bool wait, std::function<void(CommandResult&&)> callback) {
	std::shared_ptr<Command> cmd = std::make_shared<Command>();

	cmd->type = CommandType::ExecuteFunction;
	cmd->fun = fun;
	cmd->callback = callback;

	return _queueCommand(cmd, wait);
}


void CX_DisplayThread::_processQueuedCommands(void) {

	std::lock_guard<std::recursive_mutex> lock(_commandQueueMutex);

	for (std::shared_ptr<Command>& cmd : _commandQueue) {

		bool success = false;

		switch (cmd->type) {
		case CommandType::SetSwapInterval:
			if (Private::glfwContextManager.isLockedByThisThread()) {
				unsigned int swapInterval = cmd->values["swapInterval"];
				swapInterval = Util::clamp<unsigned int>(swapInterval, 0, 1);
				glfwSwapInterval(swapInterval);
				success = true;
			}
			break;

		//case CommandType::SetGLFinishAfterSwap:
			//_glFinishAfterSwap = cmd->values["finish"];
			//success = true;
			//break;

		//case CommandType::SetSwapPeriod:
			//_config.nominalFramePeriod = cmd->values["period"];
			//success = true;
			//break;

		case CommandType::AcquireRenderingContext:
		{
			bool acquire = cmd->values["acquire"];
			success = _acquireRenderingContext(acquire);
		}
			break;

		case CommandType::ExecuteFunction:
			try {
				success = cmd->fun();
				//success = true;
			} catch (...) {
				Instances::Log.error("CX_DisplayThread") << "ExecuteFunction command failed because the function threw an exception.";
				success = false;
			}
			break;
		}

		if (cmd->callback != nullptr) {
			auto cbCopy = cmd->callback;
			CommandResult result{ std::move(*cmd), success };

			cbCopy(std::move(result));
		}

	}

	_commandQueue.clear();

}



bool CX_DisplayThread::threadOwnsRenderingContext(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return Private::glfwContextManager.getLockingThreadId() == _thread.get_id();

	//return _threadOwnsRenderingContext;
}

bool CX_DisplayThread::frameQueueEnabled(void) {
	return threadOwnsRenderingContext();
}

bool CX_DisplayThread::enableFrameQueue(bool enable) {
	if (!isThreadRunning()) {
		Instances::Log.notice("CX_DisplayThread") << "enableFrameQueue(): Thread not running, returning.";
		return false; // must be running
	}

	if (!Private::glfwContextManager.isMainThread()) {
		Instances::Log.notice("CX_DisplayThread") << "enableFrameQueue(): Called from non-main thread, returning.";
		return false; // can't call from non-main thread
	}

	std::thread::id tid = _thread.get_id();
	std::thread::id lockID = Private::glfwContextManager.getLockingThreadId();
	bool isLockedByDisplayThread = tid == lockID;

	if (isLockedByDisplayThread == enable) {
		Instances::Log.notice("CX_DisplayThread") << "enableFrameQueue(): Frame queue state not changed.";
		return true;
	} else {
		Instances::Log.notice("CX_DisplayThread") << "enableFrameQueue(): Changing frame queue state.";
	}

	//if (enable == frameQueueEnabled()) {
	//	Instances::Log.notice("CX_DisplayThread") << "enableFrameQueue(): Frame queue state not changed.";
	//	return true;
	//}

	Private::CX_GlfwContextManager& cm = Private::glfwContextManager;

	if (enable) {

		if (cm.isLockedByAnyThread()) {
			//Instances::Log.notice("CX_DisplayThread") << "enableFrameQueue(): Context is locked by a thread.";
			if (cm.isLockedByThisThread()) {
				cm.unlock(); // unlock context
				Instances::Log.notice("CX_DisplayThread") << "enableFrameQueue(): Context unlocked by main thread.";
			} else {
				Instances::Log.error("CX_DisplayThread") << "enableFrameQueue(): Context was locked by another thread. It won't be unlocked.";
				return false; // can't unlock what another thread holds
			}
		}

		// tell thread to lock context
		if (!commandAcquireRenderingContext(true, true)) {
			Instances::Log.error("CX_DisplayThread") << "enableFrameQueue(): Command acquire rendering context (true) failed.";
			return false;
		} else {
			Instances::Log.notice("CX_DisplayThread") << "enableFrameQueue(): Command acquire rendering context (true) command completed successfully.";
		}
		

	} else {

		// tell thread to unlock context
		if (!commandAcquireRenderingContext(false, true)) {
			Instances::Log.error("CX_DisplayThread") << "enableFrameQueue(): Command acquire rendering context (false) failed.";
			return false;
		} else {
			Instances::Log.notice("CX_DisplayThread") << "enableFrameQueue(): Command acquire rendering context (false) completed successfully.";
		}

		if (cm.isUnlocked()) {
			cm.lock();
			Instances::Log.notice("CX_DisplayThread") << "enableFrameQueue(): Rendering context locked by main thread.";
		} else {
			Instances::Log.error("CX_DisplayThread") << "enableFrameQueue(): Rendering context was not unlocked when main thread tried to lock it. Failure.";
			return false;
		}

	}

	return enable == frameQueueEnabled();
}

bool CX_DisplayThread::_acquireRenderingContext(bool acquire) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	if (acquire == threadOwnsRenderingContext()) {
		Instances::Log.notice("CX_DisplayThread") << "_acquireRenderingContext(): Rendering context state not changed.";
		return true;
	}

	Private::CX_GlfwContextManager& cm = Private::glfwContextManager;

	if (acquire) {
		if (!cm.trylock()) {
			Instances::Log.notice("CX_DisplayThread") << "_acquireRenderingContext(): Rendering context could not be locked by display thread.";
			return false;
		} else {
			Instances::Log.notice("CX_DisplayThread") << "_acquireRenderingContext(): Rendering context locked by display thread.";
			//_threadOwnsRenderingContext = true;
		}
	} else {
		if (cm.isLockedByThisThread()) {
			cm.unlock();
			Instances::Log.notice("CX_DisplayThread") << "_acquireRenderingContext(): Rendering context was locked by display thread. It was unlocked.";
		} else {
			Instances::Log.notice("CX_DisplayThread") << "_acquireRenderingContext(): Rendering context was not locked by display thread. It was not touched.";
		}
		//_threadOwnsRenderingContext = false;
	}

	return true;
}

bool CX_DisplayThread::queueFrame(std::shared_ptr<QueuedFrame> qf) {

	if (!isThreadRunning()) {
		Instances::Log.warning("CX_DisplayThread") << "Queued frame for frame number " << qf->startFrame <<
			" ignored because the display thread was not running.";
		return false;
	}

	if (!frameQueueEnabled()) {
		Instances::Log.warning("CX_DisplayThread") << "Queued frame for frame number " << qf->startFrame <<
			" ignored because frame queue was disabled.";
		return false;
	}

	if (qf->fbo == nullptr && qf->fun == nullptr) {
		// fail: not set up. This should be impossible with a good interface
		return false;
	}

	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		
		FrameNumber lastFrameNumber = _display->swapData.getLastSwapUnit();
		if (qf->startFrame <= lastFrameNumber) {
			Instances::Log.warning("CX_DisplayThread") << "Queued frame for frame number " << qf->startFrame <<
				" arrived late (on frame number " << lastFrameNumber << ") and was ignored.";
			return false;
		}
	}

	std::lock_guard<std::recursive_mutex> qfLock(_queuedFramesMutex);

	size_t existingFrameIndex = std::numeric_limits<size_t>::max();
	size_t nextGreaterIndex = 0;
	for (size_t i = 0; i < _queuedFrames.size(); i++) {
		if (_queuedFrames[i]->startFrame == qf->startFrame) {
			existingFrameIndex = i;
			break;
		}
		if (_queuedFrames[i]->startFrame < qf->startFrame) {
			nextGreaterIndex = i + 1;
		}
	}

	if (existingFrameIndex != std::numeric_limits<size_t>::max()) {

		Instances::Log.notice("CX_DisplayThread") << "Queued frame for frame number " << qf->startFrame << " replaced.";
		_queuedFrames[existingFrameIndex] = qf;

	} else if (nextGreaterIndex < _queuedFrames.size()) {

		auto it = _queuedFrames.begin() + nextGreaterIndex;
		_queuedFrames.insert(it, qf);

		Instances::Log.notice("CX_DisplayThread") << "Queued frame for frame number " << qf->startFrame << " inserted.";

	} else {
		_queuedFrames.push_back(qf);

		Instances::Log.notice("CX_DisplayThread") << "Queued frame for frame number " << qf->startFrame << " appended.";
	}

	return true;
}

bool CX_DisplayThread::queueFrame(FrameNumber startFrame, std::function<void(void)> fun, std::function<void(QueuedFrameResult&&)> frameCompleteCallback) {

	auto qf = std::make_shared<QueuedFrame>();
	qf->startFrame = startFrame;
	qf->fun = fun;
	qf->frameCompleteCallback = frameCompleteCallback;

	return queueFrame(qf);
}

bool CX_DisplayThread::queueFrame(FrameNumber startFrame, std::shared_ptr<ofFbo> fbo, std::function<void(QueuedFrameResult&&)> frameCompleteCallback) {

	auto qf = std::make_shared<QueuedFrame>();
	qf->startFrame = startFrame;
	qf->fbo = fbo;
	qf->frameCompleteCallback = frameCompleteCallback;

	return queueFrame(qf);
}

bool CX_DisplayThread::requeueFrame(FrameNumber oldFrame, FrameNumber newFrame) {
	std::lock_guard<std::recursive_mutex> qfLock(_queuedFramesMutex);

	unsigned int oldFrameIndex = _queuedFrames.size();
	unsigned int newFrameIndex = _queuedFrames.size();
	for (unsigned int i = 0; i < _queuedFrames.size(); i++) {
		if (_queuedFrames[i]->startFrame == oldFrame) {
			oldFrameIndex = i;
		}
		if (_queuedFrames[i]->startFrame == newFrame) {
			newFrameIndex = i;
		}
	}

	if (oldFrameIndex == _queuedFrames.size()) {
		Instances::Log.warning("CX_DisplayThread") << "requeueFrame(): Nothing queued for frame " << oldFrame << ".";
		return false;
	}

	if (newFrameIndex != _queuedFrames.size()) {
		Instances::Log.warning("CX_DisplayThread") << "requeueFrame(): Frame queued for frame " <<
			newFrame << " was replaced with the frame queued for frame " << oldFrame << ".";
	}

	_queuedFrames[newFrameIndex] = _queuedFrames[oldFrameIndex];
	_queuedFrames.erase(_queuedFrames.begin() + oldFrameIndex);

	return true;
}

bool CX_DisplayThread::requeueAllFrames(int offset) {
	// unimplemented
	return false;
}

unsigned int CX_DisplayThread::getQueuedFrameCount(void) {
	std::lock_guard<std::recursive_mutex> qfLock(_queuedFramesMutex);
	return _queuedFrames.size();
}

void CX_DisplayThread::clearQueuedFrames(void) {
	std::lock_guard<std::recursive_mutex> qfLock(_queuedFramesMutex);
	_queuedFrames.clear();
}

std::shared_ptr<CX_DisplayThread::QueuedFrame> CX_DisplayThread::getQueuedFrame(unsigned int index) {
	std::lock_guard<std::recursive_mutex> qfLock(_queuedFramesMutex);

	if (index >= _queuedFrames.size()) {
		return nullptr;
	}

	return _queuedFrames[index];
}

/*
std::vector<uint64_t> CX_DisplayThread::getQueuedFrameNumbers(void) {
	std::lock_guard<std::recursive_mutex> qfLock(_queuedFramesMutex);


	std::vector<uint64_t> rval;
	for (const auto& fr : _queuedFrames) {
		rval.push_back(fr->startFrame);
	}

	return rval;
}
*/

void CX_DisplayThread::_queuedFrameTask(void) {
	if (!frameQueueEnabled()) {
		return;
	}

	_drawQueuedFrameIfNeeded();

	std::lock_guard<std::recursive_mutex> qfLock(_queuedFramesMutex);
	if (_currentQF.frame != nullptr && _currentQF.fenceSync.isSyncing()) {
		_currentQF.fenceSync.updateSync();
	}
}

void CX_DisplayThread::_queuedFramePostSwapTask(void) {

	Sync::SwapData lastSwap = _display->swapData.getLastSwapData();

	std::lock_guard<std::recursive_mutex> qfLock(_queuedFramesMutex);

	if (_currentQF.frame == nullptr) {
		_drawQueuedFrameIfNeeded();
		return;
	}

	if (_currentQF.frame->frameCompleteCallback != nullptr) {

		QueuedFrameResult result;

		result.desiredStartFrame = _currentQF.frame->startFrame;

		result.actualStartFrame = lastSwap.unit;
		result.startTime = lastSwap.time;

		result.renderTimeValid = _currentQF.fenceSync.syncSuccess();

		if (result.renderTimeValid) {
			result.renderCompleteTime = _currentQF.fenceSync.getCompleteTime();
		}

		_currentQF.frame->frameCompleteCallback(std::move(result));

	}

	_currentQF.frame = nullptr;

	_drawQueuedFrameIfNeeded();
}



void CX_DisplayThread::_drawQueuedFrameIfNeeded(void) {

	std::unique_lock<std::recursive_mutex> lock(_mutex, std::defer_lock);
	std::unique_lock<std::recursive_mutex> qfLock(_queuedFramesMutex, std::defer_lock);
	std::lock(lock, qfLock);

	FrameNumber nextFrameNumber = _display->swapData.getNextSwapUnit();

	while (_queuedFrames.size() > 0 && _queuedFrames.front()->startFrame < nextFrameNumber) {
		// this should never happen
		Instances::Log.error("CX_DisplayThread") << "Queued frame " << _queuedFrames.front()->startFrame << " was lost because its start frame was missed.";
		_queuedFrames.pop_front(); 
	}

	if (_queuedFrames.size() == 0) {
		return;
	}

	if (_queuedFrames.front()->startFrame == nextFrameNumber) {
		_currentQF.frame = _queuedFrames.front();
		_queuedFrames.pop_front();
	} else {
		return; // nothing to present
	}


	if (!Private::glfwContextManager.isLockedByThisThread()) {
		// Error: Context is unavailable
		Instances::Log.error("CX_DisplayThread") << "Rendering context unavailable.";
		return;
	}

	
	_display->beginDrawingToBackBuffer();

	if (_currentQF.frame->fbo != nullptr) {
		ofPushStyle();
		ofDisableAlphaBlending();
		ofSetColor(255);
		_currentQF.frame->fbo->draw(0, 0);
		ofPopStyle();
	} else if (_currentQF.frame->fun != nullptr) {
		_currentQF.frame->fun();
	}

	_display->endDrawingToBackBuffer();

	_currentQF.fenceSync.startSync();
}

// Display thread locking.
// These functions may only be called from the main thread.
// CX_Display does not need to have a lock to modify the display thread.
bool CX_DisplayThread::tryLock(std::string lockOwner) {
	if (lockOwner == "UNLOCKED") {
		return false;
	}
	if (isLocked()) {
		return false;
	}
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	_lockOwner = lockOwner;
	return true;
}

bool CX_DisplayThread::isLocked(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	return _lockOwner != "UNLOCKED";
}

std::string CX_DisplayThread::getLockOwner(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	return _lockOwner;
}

void CX_DisplayThread::unlock(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	_lockOwner = "UNLOCKED";
}

} //namespace CX