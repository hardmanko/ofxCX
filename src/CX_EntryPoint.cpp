#include "CX_EntryPoint.h"

#include "CX_Private.h"

#include "CX_GLFWWindow_Compat.h"

/*! An instance of CX::CX_Display that is hooked into the CX backend.
\ingroup entryPoint */
CX::CX_Display CX::Instances::Display;

/*! An instance of CX_InputManager that is hooked into the CX backend.
\ingroup entryPoint */
CX::CX_InputManager CX::Instances::Input; 

namespace CX {
	namespace Private {
		
		class App {
		public:
			void setup(void);
			void setupWindow(CX_WindowConfiguration_t config);
		};
		
		//Apparently oF has added this, so expect to remove it when oF version 0.9.0 is supported.
		void glfwErrorCallback(int code, const char* message) {
			CX::Instances::Log.error("ofAppGLFWWindow") << "GLFW error code: " << code << " " << message;
		}
	}
}


void CX::Private::App::setup (void) {

	ofSetWorkingDirectoryToDefault();

	CX::Instances::Log.captureOFLogMessages();
	CX::Instances::Log.levelForAllModules(CX_LogLevel::LOG_NOTICE);

	Util::checkOFVersion(0, 8, 0); //Check to make sure that the version of oF that is being used is supported by CX.

	setupWindow(CX::CX_WindowConfiguration_t());

	CX::Instances::Input.pollEvents(); //So that the window is at least minimally responding
		//This must happen after the window is configured because it relies on GLFW.

	//Why use these? CX has RNG and Clock.
	ofSeedRandom();
	ofResetElapsedTimeCounter();

	CX::Instances::Display.setup();

	Clock.precisionTest(10000);

	CX::Instances::Log.flush(); //Flush logs after setup, so user can see if any errors happened during setup.
}

void CX::Private::App::setupWindow(CX_WindowConfiguration_t config) {

	glfwSetErrorCallback(&glfwErrorCallback);


	CX::Private::CX_GLVersion glver(1,0,0);
	
	//Find out what version of openGL the graphics card supports, which requires the creation 
	//of a GLFW window (or other initialization of openGL).
	glfwInit();
	GLFWwindow *windowP;
	glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
	windowP = glfwCreateWindow(1, 1, "", NULL, NULL);
	glfwMakeContextCurrent(windowP);
	glver = CX::Private::getOpenGLVersion();
	glfwDestroyWindow(windowP);
	glfwWindowHint(GLFW_VISIBLE, GL_TRUE);
	
	

	ofPtr<ofAppGLFWCompatibilityWindow> window(new ofAppGLFWCompatibilityWindow);
	window->setOpenGLVersion(glver.major, glver.minor);
	window->setNumSamples(CX::Util::getSampleCount());

	//Check to see if the OpenGL version is high enough to fully support ofGLProgrammableRenderer. If not, fall back on ofGLRenderer.
	if (glver.major >= 3 && glver.minor >= 2) {
		ofSetCurrentRenderer(ofPtr<ofBaseRenderer>(new ofGLProgrammableRenderer), true);
	} else {
		ofSetCurrentRenderer(ofPtr<ofBaseRenderer>(new ofGLRenderer), true);
	}
	ofSetupOpenGL(ofPtr<ofAppBaseWindow>(window), config.width, config.height, config.mode);

	ofGetCurrentRenderer()->update(); //Only needed for ofGLRenderer, not for ofGLProgrammableRenderer

	window->initializeWindow();
	window->setWindowTitle("CX Experiment");

	CX::Private::glfwContext = glfwGetCurrentContext();

	//For some reason, this is needed in order to get text to display properly: (not any more, apparently)
	//ofSetOrientation(ofOrientation::OF_ORIENTATION_DEFAULT, true); 
}

int main (void) {	
	CX::Private::App A;
	A.setup();
	runExperiment();
	return 0;
}