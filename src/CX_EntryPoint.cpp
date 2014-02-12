#include "CX_EntryPoint.h"

#include "CX_Private.h"

#ifdef CX_USE_VIDEO_HW_COMPAT
#include "CX_GLFWWindow_Compat.h"
#else
#include "ofAppGLFWWindow.h"
#endif

/*! An instance of CX::CX_Display that is hooked into the CX backend.
\ingroup entryPoint */
CX::CX_Display CX::Instances::Display;

/*! An instance of CX_InputManager that is hooked into the CX backend.
\ingroup entryPoint */
CX::CX_InputManager CX::Instances::Input; 

namespace CX {
	namespace Private {

		struct CX_WindowConfiguration_t {
			CX_WindowConfiguration_t(void) :
			width(800),
			height(600),
			mode(ofWindowMode::OF_WINDOW)
			{}

			int width;
			int height;

			ofWindowMode mode;
		};

		
		class App {
		public:
			void setup(void);
			void setupWindow(CX_WindowConfiguration_t config);

			//void update(void);
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
#ifdef CX_DEBUG
	CX::Instances::Log.levelForAllModules(CX_LogLevel::LOG_ALL);
	CX::Instances::Log.levelForFile(CX_LogLevel::LOG_ALL, "Last run.txt");
#else
	CX::Instances::Log.levelForAllModules(CX_LogLevel::LOG_NOTICE);
#endif

	Util::checkOFVersion(0, 8, 0); //Check to make sure that the version of oF that is being used is supported by CX.

	setupWindow(CX::Private::CX_WindowConfiguration_t());

	CX::Instances::Input.pollEvents(); //So that the window is at least minimally responding
		//This must happen after the window is condifured because it relies on GLFW.

	//Why use these? CX has RNG and Clock.
	ofSeedRandom();
	ofResetElapsedTimeCounter();

	CX::Instances::Display.setup();

	Clock.precisionTest();

	CX::Instances::Log.flush(); //Flush logs after setup, so user can see if any errors happened during setup.
}

void CX::Private::App::setupWindow(CX_WindowConfiguration_t config) {

	glfwSetErrorCallback(&glfwErrorCallback);

#ifdef CX_DEBUG
	CX::Instances::Log.notice() << "Error callback set";
	CX::Instances::Log.flush();
#endif

#ifdef CX_USE_VIDEO_HW_COMPAT
	ofPtr<ofAppGLFWCompatibilityWindow> window(new ofAppGLFWCompatibilityWindow);
	window->setGLSLVersion(CX_GLSL_VERSION_MAJOR, CX_GLSL_VERSION_MINOR);
	window->setOpenGLVersion(CX_GL_VERSION_MAJOR, CX_GL_VERSION_MINOR);
#else
	ofPtr<ofAppGLFWWindow> window(new ofAppGLFWWindow);
#endif
	
	window->setNumSamples(CX::Util::getSampleCount());

#ifdef CX_DEBUG
	CX::Instances::Log.notice() << "Window constructed";
	CX::Instances::Log.flush();
#endif

	ofSetCurrentRenderer(ofPtr<ofBaseRenderer>(new ofGLRenderer), true);
	ofSetupOpenGL(ofPtr<ofAppBaseWindow>(window), config.width, config.height, config.mode);

#ifdef CX_DEBUG
	CX::Instances::Log.notice() << "OpenGL set up";
	CX::Instances::Log.flush();
#endif

	ofGetCurrentRenderer()->update(); //Only needed for ofGLRenderer, not for ofGLProgrammableRenderer

	window->initializeWindow();

#ifdef CX_DEBUG
	CX::Instances::Log.notice() << "Window initialized";
	CX::Instances::Log.flush();
#endif

	CX::Private::glfwContext = glfwGetCurrentContext();

	ofSetOrientation(ofOrientation::OF_ORIENTATION_DEFAULT, true); //For some reason, this is need in order to get text to display properly.
}

int main (void) {	
	CX::Private::App A;
	A.setup();
	runExperiment();
	return 0;
}