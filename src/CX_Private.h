#pragma once

#include <sstream>
#include <string>

#include "ofUtils.h"
#include "ofEvents.h"
#include "ofAppGLFWWindow.h"

#include "GLFW/glfw3.h"




namespace CX {

	// Forward declaration
	struct CX_GLVersion;

/*! The Private namespace contains symbols that may be visible in user code but which should not be used by user code.
*/
namespace Private {

	// Maybe CX_RenderingContextManager
	class CX_GlfwContextManager {
	public:

		void setup(GLFWwindow* context, std::thread::id mainThreadId); // Do not call: Only called in CX_EntryPoint

		bool trylock(void);
		void lock(void); // blocks until it can get a lock
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

	extern std::shared_ptr<ofAppBaseWindow> appWindow;

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
