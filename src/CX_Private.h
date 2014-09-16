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
//so apparently sysmacros.h defines some macros with these names on Linux... f
#ifdef major
#undef major
#endif

#ifdef minor
#undef minor
#endif
#endif

namespace CX {

/*! This namespace contains symbols that may be visible in user code but which should not be used by user code.
*/
namespace Private {
	extern GLFWwindow* glfwContext;

	extern ofPtr<ofAppBaseWindow> appWindow;

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


}
}
