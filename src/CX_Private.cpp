#include "CX_Private.h"

#include "GLFW\glfw3.h"
GLFWwindow *CX::Private::glfwContext = NULL;

ofPtr<CX::Private::CX_AppWindow> CX::Private::window;

CX::Private::CX_GLVersion CX::Private::getOpenGLVersion(void) {
	static CX_GLVersion ver = []()->CX_GLVersion {
		std::string s = (char*)glGetString(GL_VERSION);

		vector<string> versionVendor = ofSplitString(s, " "); //Vendor specific information follows a space.
		vector<string> version = ofSplitString(versionVendor[0], "."); //Version numbers

		CX_GLVersion v;
		v.major = ofToInt(version[0]);
		v.minor = ofToInt(version[1]);
		if (version.size() == 3) {
			v.release = ofToInt(version[2]);
		} else {
			v.release = 0;
		}

		return v;
	}();

	return ver;
}


//The version is encoded as 330 for version 3.3.0

int getGLVersionInt(void) {
	static int version = [](void) -> int {
		CX::Private::CX_GLVersion ver = CX::Private::getOpenGLVersion();
		int version = 100 * ver.major + 10 * ver.minor + ver.release;
		return version;
	}();

	return version;
}

/*
int CX::Private::getGLSLVersion(void) {
	static int glslVersion = [](void) -> int {
		int glv = getGLVersionInt();

		if (glv >= 330) {
			return glv;
		} else if (glv < 200) {
			return -1; //No version exists
		}

		switch (glv) {
		case 200: return 110;
		case 210: return 120;
		case 300: return 130;
		case 310: return 140;
		case 320: return 150;
		default: return -1;
		}

		return -1;

	}();

	return glslVersion;
}
*/

CX::Private::CX_GLVersion CX::Private::getGLSLVersionFromGLVersion(CX::Private::CX_GLVersion glVersion) {
	if (glVersion.major >= 3 && glVersion.minor >= 3) {
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

CX::Private::CX_GLVersion CX::Private::getGLSLVersion(void) {
	static CX_GLVersion ver = getGLSLVersionFromGLVersion(getOpenGLVersion());

	return ver;
}

bool CX::Private::glFenceSyncSupported(void) {
	return glVersionAtLeast(3,2,0); //Fence sync is also supported by ARB_sync, but that means dealing with potentially device-specific implementations.
}


bool CX::Private::glVersionAtLeast(int desiredMajor, int desiredMinor, int desiredRelease) {
	CX_GLVersion actual = getOpenGLVersion();
	CX_GLVersion desired(desiredMajor, desiredMinor, desiredRelease);

	return glCompareVersions(actual, desired) >= 0;
}

//Returns 1 of a > b, 0 if b == a, or -1 if a < b
int CX::Private::glCompareVersions(CX_GLVersion a, CX_GLVersion b) {
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

