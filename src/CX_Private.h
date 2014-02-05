
//#ifdef TARGET_WIN32
//#include "Windows.h" //Must include Windows.h before glfw3.h?
//#endif

//struct GLFWwindow;
#include "GLFW\glfw3.h"

namespace CX {
	namespace Private {
		extern GLFWwindow *glfwContext;
	}
}