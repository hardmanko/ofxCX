#include "CX_Private.h"

#include "GLFW\glfw3.h"
GLFWwindow *CX::Private::glfwContext = NULL;




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


int CX::Private::getGLSLVersion(void) {
	static int glslVersion = [](void) -> int {
		int glv = getGLVersionInt();

		int glslv;

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

bool CX::Private::glFenceSyncSupported(void) {
	return glVersionAtLeast(3,2,0); //Fence sync is also supported by ARB_sync, but that means dealing with potentially device-specific implementations.
}


bool CX::Private::glVersionAtLeast(int desiredMajor, int desiredMinor, int desiredRevision) {
	CX_GLVersion actual = getOpenGLVersion();
	if (actual.major > desiredMajor) {
		return true;
	} else if (actual.major == desiredMajor) {
		if (actual.minor > desiredMinor) {
			return true;
		} else if (actual.minor == desiredMinor) {
			if (actual.release >= desiredRevision) {
				return true;
			}
		}
	}
	return false;
}