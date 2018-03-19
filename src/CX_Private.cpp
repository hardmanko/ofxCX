#include "CX_Private.h"

#include "GLFW/glfw3.h"

#include "CX_Mouse.h"

namespace CX {
namespace Private {

////////////////////
// CX_GLFenceSync //
////////////////////

CX_GLFenceSync::CX_GLFenceSync(void) :
	_syncSuccess(false),
	_syncCompleteTime(-1),
	_syncStart(-1),
	_status(SyncStatus::NotStarted)
{}

void CX_GLFenceSync::startSync(void) {

	stopSyncing();

	_fenceSyncObject = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glFlush(); //This glFlush assures that the fence sync object gets pushed into the command queue.

	_syncStart = Instances::Clock.now();

	_status = SyncStatus::Syncing;
}

void CX_GLFenceSync::updateSync(void) {

	if (_status != SyncStatus::Syncing) {
		return;
	}

	GLenum result = glClientWaitSync(_fenceSyncObject, 0, 0);
	if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {

		_syncCompleteTime = Instances::Clock.now();
		_status = SyncStatus::SyncComplete;
		_syncSuccess = true;
		
		
	} else if (result == GL_WAIT_FAILED) {

		_syncCompleteTime = CX_Millis(-1);
		_status = SyncStatus::SyncComplete;
		_syncSuccess = false;
		
	} 
	//else if (result == GL_TIMEOUT_EXPIRED) {
		// do nothing
	//}

}

void CX_GLFenceSync::stopSyncing(void) {
	glDeleteSync(_fenceSyncObject);
	_status = SyncStatus::NotStarted;
}

void CX_GLFenceSync::clear(void) {
	stopSyncing();
	_syncSuccess = false;
	_syncCompleteTime = CX_Millis(-1);
	_syncStart = CX_Millis(-1);
}

bool CX_GLFenceSync::isSyncing(void) const {
	return _status == SyncStatus::Syncing;
}

bool CX_GLFenceSync::syncSuccess(void) const {
	return _status == SyncStatus::SyncComplete && _syncSuccess;
}

bool CX_GLFenceSync::syncComplete(void) const {
	return _status == SyncStatus::SyncComplete;
}

CX_Millis CX_GLFenceSync::getStartTime(void) const {
	return _syncStart;
}

CX_Millis CX_GLFenceSync::getSyncTime(void) const {
	return _syncCompleteTime;
}


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

		glfwMakeContextCurrent(_glfwContext);
		//glFinish();

		// This extra release and acquire appears to be required, probably due to bug, maybe due to nasty GLFW and oF interaction
		glfwMakeContextCurrent(NULL);
		//glFinish();
		glfwMakeContextCurrent(_glfwContext);
		//glFinish();

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

CX_GlfwContextManager glfwContextManager;

GLFWwindow* glfwContext = NULL;




ofPtr<ofAppBaseWindow> appWindow;

CX_GLVersion glVersion;


//Find out what version of openGL the graphics card supports, which requires the creation
//of a GLFW window (or other initialization of openGL).
void learnOpenGLVersion(void) {

	glfwInit();
	GLFWwindow *windowP;
	glfwWindowHint(GLFW_VISIBLE, GL_FALSE); //Make the window invisible
	windowP = glfwCreateWindow(1, 1, "", NULL, NULL); //Create the window
	glfwMakeContextCurrent(windowP);

	//Once GL is initialized, get the version number from the version number string.
	std::string s = (char*)glGetString(GL_VERSION);

	std::vector<std::string> versionVendor = ofSplitString(s, " "); //Vendor specific information follows a space, so split it off.
	std::vector<std::string> version = ofSplitString(versionVendor[0], "."); //Version numbers

	glVersion.major = ofToInt(version[0]);
	glVersion.minor = ofToInt(version[1]);
	if (version.size() == 3) {
		glVersion.release = ofToInt(version[2]);
	} else {
		glVersion.release = 0;
	}

	glfwDestroyWindow(windowP);
	glfwWindowHint(GLFW_VISIBLE, GL_TRUE); //Make the next created window visible

}

CX_GLVersion getOpenGLVersion(void) {
	return glVersion;
}


CX_GLVersion getGLSLVersionFromGLVersion(CX_GLVersion glVersion) {
	if (glVersionAtLeast(3, 2, 0)) {
		return glVersion;
	} else if (glVersion.major < 2) {
		return CX_GLVersion(0, 0, 0); //No version exists
	} else if (glVersion.major == 2 && glVersion.minor == 0) {
		return CX_GLVersion(1, 10, 59);
	} else if (glVersion.major == 2 && glVersion.minor == 1) {
		return CX_GLVersion(1, 20, 8);
	} else if (glVersion.major == 3 && glVersion.minor == 0) {
		return CX_GLVersion(1, 30, 10);
	} else if (glVersion.major == 3 && glVersion.minor == 1) {
		return CX_GLVersion(1, 40, 8);
	} else if (glVersion.major == 3 && glVersion.minor == 2) {
		return CX_GLVersion(1, 50, 11);
	}

	return CX_GLVersion(0, 0, 0); //No version exists
}

CX_GLVersion getGLSLVersion(void) {
	static CX_GLVersion ver = getGLSLVersionFromGLVersion(getOpenGLVersion());

	return ver;
}

bool glFenceSyncSupported(void) {
	return glVersionAtLeast(3, 2, 0); //Fence sync is also supported by ARB_sync, but that means dealing with potentially device-specific implementations.
}


bool glVersionAtLeast(int desiredMajor, int desiredMinor, int desiredRelease) {
	CX_GLVersion actual = getOpenGLVersion();
	CX_GLVersion desired(desiredMajor, desiredMinor, desiredRelease);

	return glCompareVersions(actual, desired) >= 0;
}

//Returns 1 if a > b, 0 if b == a, or -1 if a < b
int glCompareVersions(CX_GLVersion a, CX_GLVersion b) {
	if (a.major > b.major) {
		return 1;
	} else if (a.major < b.major) {
		return -1;
	} else {
		if (a.minor > b.minor) {
			return 1;
		} else if (a.minor < b.minor) {
			return -1;
		} else {
			if (a.release > b.release) {
				return 1;
			} else if (a.release < b.release) {
				return -1;
			} else {
				return 0;
			}
		}
	}

	return 0;
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
