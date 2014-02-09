#pragma once

//This file should not be included in a location that will make its contents visible to user code.

#include "GLFW\glfw3.h"

namespace CX {
	namespace Private {
		extern GLFWwindow *glfwContext;
	}
}