#include "CX_Private.h"

#include "GLFW/glfw3.h"

#include "CX_Mouse.h"

namespace CX {
namespace Private {

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
		} else if (a.minor > b.minor) {
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
	s = ofToLower(s);
	if ((s == "false") || (s.front() == '0')) {
		return 0;
	} else if ((s == "true") || (s.front() == '1')) {
		return 1;
	}

	CX::Instances::Log.error("Private") << "stringToBooleint: Failure attempting to convert "
		"string to boolean: invalid boolean value given: \"" << s << "\". Use \"0\", \"1\", \"true\", or \"false\".";
	return -1;
}

} //namespace Private
} //namespace CX