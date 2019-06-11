#include "CX_Private.h"

#include "CX_Logger.h"
#include "CX_EntryPoint.h"

namespace CX {
namespace Private {


	CX_GlfwContextManager glfwContextManager;

	std::shared_ptr<ofAppBaseWindow> appWindow;


///////////////////////////
// CX_GlfwContextManager //
///////////////////////////

void CX_GlfwContextManager::setup(GLFWwindow* context, std::thread::id mainThreadId) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_glfwContext = context;
	_mainThreadId = mainThreadId;

	this->lock();

}

bool CX_GlfwContextManager::trylock(void) {

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

void CX_GlfwContextManager::lock(void) {
	while (!trylock())
		;
}

void CX_GlfwContextManager::unlock(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	if (isLockedByThisThread()) {
		_lockingThreadId = std::thread::id();
		glfwMakeContextCurrent(NULL);
		//glFinish();
	}
}

/*
bool CX_GlfwContextManager::isCurrentOnThisThread(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _glfwContext == glfwGetCurrentContext(); // glfwGetCurrentContext returns the context of any thread.
}
*/

bool CX_GlfwContextManager::isUnlocked(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _lockingThreadId == std::thread::id();
}

bool CX_GlfwContextManager::isLockedByThisThread(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _lockingThreadId == std::this_thread::get_id();
}

bool CX_GlfwContextManager::isLockedByMainThread(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _lockingThreadId == _mainThreadId;
}

bool CX_GlfwContextManager::isLockedByAnyThread(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _lockingThreadId != std::thread::id();
}

std::thread::id CX_GlfwContextManager::getLockingThreadId(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _lockingThreadId;
}

GLFWwindow* CX_GlfwContextManager::get(void) {
	GLFWwindow* rval = nullptr;
	if (isLockedByThisThread()) {
		rval = _glfwContext;
	}
	return rval;
}

bool CX_GlfwContextManager::isMainThread(void) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	return std::this_thread::get_id() == _mainThreadId;
}


/* Returns 0 if the string evaluates to false, 1 if the string evaluates to true.
If the string evaluates to neither, this returns -1 and logs an error. */
int stringToBooleint(std::string s) {
	s = ofTrim(s);
	s = ofToLower(s);
	if ((s == "false") || (s.front() == '0')) {
		return 0;
	} else if ((s == "true") || (s.front() == '1')) {
		return 1;
	}

	CX::Instances::Log.error("Private") << "stringToBooleint: Failure attempting to convert "
		"string to boolean: invalid boolean value given: \"" << s << "\". Use \"0\", \"1\", \"true\", or \"false\" (capitalization is ignored).";
	return -1;
}

#ifdef TARGET_WIN32
namespace Windows {
	std::string convertErrorCodeToString(DWORD errorCode) {

		if (errorCode == 0) {
			return "No error.";
		}

		LPSTR messageBuffer = nullptr;
		size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
									 NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

		std::string message(messageBuffer, size);

		LocalFree(messageBuffer); //Free the buffer.

		return message;
	}

	bool setProcessToHighPriority(void) {
		//See https://msdn.microsoft.com/en-us/library/ms686219%28v=vs.85%29.aspx

		DWORD dwError;
		DWORD dwPriClass;

		HANDLE thisProcess = GetCurrentProcess();

		if (!SetPriorityClass(thisProcess, HIGH_PRIORITY_CLASS)) {
			dwError = GetLastError();
			CX::Instances::Log.error() << "Error setting process priority: " << convertErrorCodeToString(dwError);
			return false;
		}

		dwPriClass = GetPriorityClass(GetCurrentProcess());

		if (dwPriClass != HIGH_PRIORITY_CLASS) {
			CX::Instances::Log.error() << "Failed to set priority to high.";
			return false;
		}
		return true;
	}
} //namespace Windows
#endif

} //namespace Private
} //namespace CX
