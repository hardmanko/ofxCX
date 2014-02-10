#include "CX_EntryPoint.h"

#include "CX_Private.h"
#include "ofAppGLFWWindow.h"

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

	Util::checkOFVersion(0, 8, 0); //Check to make sure that the version of oF that is being used is supported by CX.

	ofSetWorkingDirectoryToDefault();

	setupWindow(CX::Private::CX_WindowConfiguration_t());

	//Why use these? CX has RNG and Clock.
	ofSeedRandom();
	ofResetElapsedTimeCounter();

	CX::Instances::Display.setup();

	CX::Instances::Log.captureOFLogMessages();
	CX::Instances::Log.levelForAllModules(CX_LogLevel::LOG_NOTICE);

	Clock.precisionTest();

	//Log.levelForConsole(CX_LogLevel::LOG_ALL);
	//Log.levelForFile(CX_LogLevel::LOG_ALL);
	//Log.levelForFile(CX_LogLevel::LOG_ALL, "Log for last run.txt");

	CX::Instances::Log.flush(); //Flush logs after setup, so user can see if any errors happened their setup.
}

void CX::Private::App::setupWindow(CX_WindowConfiguration_t config) {

	glfwSetErrorCallback(&glfwErrorCallback);

	ofPtr<ofAppGLFWWindow> window(new ofAppGLFWWindow);
	window->setNumSamples(CX::Util::getSampleCount());

	ofSetCurrentRenderer(ofPtr<ofBaseRenderer>(new ofGLRenderer), true);
	ofSetupOpenGL(ofPtr<ofAppBaseWindow>(window), config.width, config.height, config.mode);

	ofGetCurrentRenderer()->update(); //Only needed for ofGLRenderer, not for ofGLProgrammableRenderer

	window->initializeWindow();

	CX::Private::glfwContext = glfwGetCurrentContext();

	ofSetOrientation(ofOrientation::OF_ORIENTATION_DEFAULT, true);
}

int main (void) {	
	CX::Private::App A;
	A.setup();
	runExperiment();
	return 0;
}