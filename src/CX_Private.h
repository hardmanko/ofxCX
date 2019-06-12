#pragma once

#include <sstream>
#include <string>

#include "ofUtils.h"
#include "ofEvents.h"
#include "ofAppGLFWWindow.h"

#include "GLFW/glfw3.h"


namespace CX {

/*! The Private namespace contains symbols that may be visible in user code but which should not be used by user code.
*/
namespace Private {

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
