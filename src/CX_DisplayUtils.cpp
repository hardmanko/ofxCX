#include "CX_DisplayUtils.h"

namespace CX {
namespace Util {

///////////////////////////
// GlfwContextManager //
///////////////////////////

void GlfwContextManager::setup(GLFWwindow* context, std::thread::id mainThreadId) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_glfwContext = context;
	_mainThreadId = mainThreadId;

	this->lock();

}

bool GlfwContextManager::trylock(void) {

	std::lock_guard<std::recursive_mutex> lock(_mutex);

	if (isUnlocked()) {
		_lockingThreadId = std::this_thread::get_id();

		// TODO: Look into this for oF 0.10.1
		glfwMakeContextCurrent(_glfwContext);
		//glFinish();

//#if !(OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR >= 10 && OF_VERSION_PATCH >= 1)
		// This extra release and acquire appears to be required, probably due to bug, maybe due to nasty GLFW and oF interaction
		glfwMakeContextCurrent(NULL);
		//glFinish();
		glfwMakeContextCurrent(_glfwContext);
		//glFinish();
//#endif

		return true;
	}

	return false;
}

void GlfwContextManager::lock(void) {
	while (!trylock())
		;
}

void GlfwContextManager::unlock(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	if (isLockedByThisThread()) {
		_lockingThreadId = std::thread::id();
		glfwMakeContextCurrent(NULL);
		//glFinish();
	}
}

/*
bool GlfwContextManager::isCurrentOnThisThread(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _glfwContext == glfwGetCurrentContext(); // glfwGetCurrentContext returns the context of any thread.
}
*/

bool GlfwContextManager::isUnlocked(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _lockingThreadId == std::thread::id();
}

bool GlfwContextManager::isLockedByThisThread(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _lockingThreadId == std::this_thread::get_id();
}

bool GlfwContextManager::isLockedByMainThread(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _lockingThreadId == _mainThreadId;
}

bool GlfwContextManager::isLockedByAnyThread(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _lockingThreadId != std::thread::id();
}

std::thread::id GlfwContextManager::getLockingThreadId(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _lockingThreadId;
}

GLFWwindow* GlfwContextManager::get(void) {
	GLFWwindow* rval = nullptr;
	if (isLockedByThisThread()) {
		rval = _glfwContext;
	}
	return rval;
}

bool GlfwContextManager::isMainThread(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	return std::this_thread::get_id() == _mainThreadId;
}

////////////////////
// GLFenceSync //
////////////////////

GLFenceSync::GLFenceSync(void) :
	//_syncSuccess(false),
	_syncComplete(-1),
	_syncStart(-1),
	_status(SyncStatus::Idle)
{}

void GLFenceSync::startSync(CX_Millis timeout) {

	clear();

	_fenceSyncObject = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glFlush(); //This glFlush assures that the fence sync object gets pushed into the command queue.
	// But maybe its redundant now that updateSync calls glClientWaitSync with flag GL_SYNC_FLUSH_COMMANDS_BIT?

	_syncStart = CX::Instances::Clock.now();
	_timeout = timeout;

	_status = SyncStatus::Syncing;
}

void GLFenceSync::updateSync(void) {

	if (_status != SyncStatus::Syncing) {
		return;
	}

	GLenum result = glClientWaitSync(_fenceSyncObject, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
	if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {

		_syncComplete = CX::Instances::Clock.now();
		_status = SyncStatus::SyncSuccess;

	}
	else if (result == GL_WAIT_FAILED) {

		// Log something? This is an OpenGL error
		_syncComplete = CX::Instances::Clock.now();
		_status = SyncStatus::SyncFailed;

	}
	//else if (result == GL_TIMEOUT_EXPIRED) {
		//do nothing. This isn't supposed to be able to happen
		//CX::Instances::Log.warning("GLFenceSync") << "Sync timeout";
	//}

	// Check for timeout after checking the wait sync to give one last
	// chance before timing out.
	if (_timeout > CX_Millis(0)) {
		CX_Millis t = CX::Instances::Clock.now();
		if (t - _syncStart > _timeout) {
			_syncComplete = t;
			_status = SyncStatus::TimedOut;
		}
	}
}

void GLFenceSync::stopSyncing(void) {
	// This may generate GL_INVALID_VALUE error if _fenceSyncObject is not initialized.
	// But its silent, so you can ignore it.
	glDeleteSync(_fenceSyncObject);

	_status = SyncStatus::Idle;
}

void GLFenceSync::clear(void) {
	stopSyncing();
	_syncComplete = CX_Millis(-1);
	_syncStart = CX_Millis(-1);
	_timeout = CX_Millis(-1);
}

bool GLFenceSync::isSyncing(void) const {
	return _status == SyncStatus::Syncing;
}

bool GLFenceSync::syncSuccess(void) const {
	return _status == SyncStatus::SyncSuccess;
}

bool GLFenceSync::syncComplete(void) const {
	return _status == SyncStatus::SyncSuccess ||
		_status == SyncStatus::SyncFailed ||
		_status == SyncStatus::TimedOut;
}

GLFenceSync::SyncStatus GLFenceSync::getStatus(void) const {
	return _status;
}

CX_Millis GLFenceSync::getStartTime(void) const {
	return _syncStart;
}

CX_Millis GLFenceSync::getCompleteTime(void) const {
	return _syncComplete;
}

} // namespace Util
} // namespace CX