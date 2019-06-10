#pragma once

//This file should not be included in a location that will make its contents visible to user code.

#include "ofUtils.h"
#include "ofEvents.h"

#include "GLFW/glfw3.h"
#include <sstream>
#include <string>

#include "CX_AppWindow.h"
#include "CX_Logger.h"


#ifdef TARGET_LINUX
//so apparently sysmacros.h defines some macros with these names on Linux...
#ifdef major
#undef major
#endif

#ifdef minor
#undef minor
#endif
#endif

namespace CX {

/*! The Private namespace contains symbols that may be visible in user code but which should not be used by user code.
*/
namespace Private {

	class CX_GLFenceSync {
	public:

		CX_GLFenceSync(void);

		void startSync(void);
		void updateSync(void);
		void stopSyncing(void);

		void clear(void);

		bool isSyncing(void) const;
		bool syncComplete(void) const;
		bool syncSuccess(void) const;

		CX_Millis getStartTime(void) const;
		CX_Millis getSyncTime(void) const;

	private:

		enum class SyncStatus {
			NotStarted,
			Syncing,
			SyncComplete
		};

		SyncStatus _status;

		bool _syncSuccess;

		GLsync _fenceSyncObject;

		CX_Millis _syncStart;
		CX_Millis _syncCompleteTime;

	};



	// Maybe CX_RenderingContextManager
	class CX_GlfwContextManager {
	public:

		void setup(GLFWwindow* context, std::thread::id mainThreadId); // Do not call: Only called in CX_EntryPoint

		bool trylock(void);
		void lock(void); // if isLockedByAnyThread() == true, it is a programming error to call lock()
		void unlock(void); // if isLockedByThisThread() == false, it is a programming error to call unlock()

		//bool isCurrentOnThisThread(void); // If the rendering context is current on the calling thread, returns true

		bool isUnlocked(void);

		bool isLockedByThisThread(void);
		bool isLockedByMainThread(void);
		bool isLockedByAnyThread(void);

		std::thread::id getLockingThreadId(void);

		GLFWwindow* get(void);

		bool isMainThread(void); // true if function is called in main thread. This doesn't really belong in this class

	private:

		std::recursive_mutex _mutex; // mutex for accessing data of this class

		std::thread::id _lockingThreadId;
		std::thread::id _mainThreadId;

		GLFWwindow* _glfwContext;
	};

	extern CX_GlfwContextManager glfwContextManager;

	extern GLFWwindow* glfwContext;


	extern std::shared_ptr<ofAppBaseWindow> appWindow;

	struct CX_GLVersion {
		CX_GLVersion(void) :
			major(0),
		    minor(0),
			release(0)
		{}

		CX_GLVersion(int maj, int min, int rel) :
			major(maj),
			minor(min),
			release(rel)
		{}

		int major;
		int minor;
		int release;
	};

	void learnOpenGLVersion(void);
	CX_GLVersion getOpenGLVersion(void);
	CX_GLVersion getGLSLVersion(void);
	bool glFenceSyncSupported(void);
	bool glVersionAtLeast(int desiredMajor, int desiredMinor, int desiredRelease = 0);
	int glCompareVersions(CX_GLVersion a, CX_GLVersion b);

	CX_GLVersion getGLSLVersionFromGLVersion(CX_GLVersion glVersion);

	int stringToBooleint(std::string s);

#ifdef TARGET_WIN32
	namespace Windows {
		std::string convertErrorCodeToString(DWORD errorCode);
		bool setProcessToHighPriority(void);
	}
#endif

	// For when you want to use a shared_ptr improperly.
	// Used like:
	// T t; // somewhere
	// shared_ptr<T> ptr = wrapPtr<T>(&t);
	// It turns something that is called std::shared_ptr<T> into something that acts like a bare pointer, T*.
	template <typename T>
	std::shared_ptr<T> wrapPtr(T* ptr) {
		auto nopDeleter = [](T* x) { return; };
		return std::shared_ptr<T>(ptr, nopDeleter);
	}

	template <typename T>
	std::shared_ptr<T> moveIntoPtr(T&& t) {
		return std::make_shared<T>(std::move(t));
	}

} // namespace Private
} // namespace CX
