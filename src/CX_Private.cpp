#include "CX_Private.h"

#include "GLFW\glfw3.h"
GLFWwindow *CX::Private::glfwContext = NULL;

struct CX_GLVersion {
	int major;
	int minor;
	int revision;
};


CX_GLVersion getGlVersion(void) {
	static CX_GLVersion ver = []()->CX_GLVersion {
		std::string s = (char*)glGetString(GL_VERSION);

		std::vector<std::string> parts = ofSplitString(s, ".", false);

		CX_GLVersion v;
		v.major = ofToInt(parts[0]);
		v.minor = ofToInt(parts[1]);
		if (parts.size() >= 3) {
			v.revision = ofToInt(parts[2]); //This is basically meaningless because no recent versions have used it. The only version to do so was 1.2.1
		} else {
			v.revision = 0;
		}
		return v;
	}();

	return ver;
}


//The version is encoded as 330 for version 3.3.0
int CX::Private::getOpenGLVersion(void) {
	static int version = [](void) -> int {
		CX_GLVersion ver = getGlVersion();
		int version = 100 * ver.major + 10 * ver.minor + ver.revision;
		return version;
	}();

	return version;
}

int CX::Private::getGLSLVersion(void) {
	static int glslVersion = [](void) -> int {
		int glv = getOpenGLVersion();

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


bool CX::Private::glVersionAtLeast(int versionMajor, int versionMinor, int versionRevision = 0) {
	CX_GLVersion ver = getGlVersion();
	if (versionMajor > ver.major) {
		return true;
	} else if (versionMajor < ver.major) {
		return false;
	} else {
		if (versionMinor >= ver.minor) {
			if (versionRevision >= ver.revision) {
				return true;
			} else {
				return false;
			}
		} else {
			return false;
		}
	}
	return false;
}