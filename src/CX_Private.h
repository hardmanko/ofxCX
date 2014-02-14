#pragma once

//This file should not be included in a location that will make its contents visible to user code.

#include "ofUtils.h"

#include "GLFW\glfw3.h"
#include <sstream>
#include <string>



namespace CX {
	namespace Private {
		extern GLFWwindow *glfwContext;

		struct CX_GLVersion {
			int major;
			int minor;
			int release;
		};

		CX_GLVersion getOpenGLVersion(void);
		int getGLSLVersion(void);
		bool glFenceSyncSupported(void);
		bool glVersionAtLeast(int versionMajor, int versionMinor, int versionRevision = 0);

	}
}