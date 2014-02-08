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

			void update(void);
			void exit(ofEventArgs &a);
		};
		

		void glfwErrorCallback(int code, const char* message) {
			CX::Instances::Log.error("ofAppGLFWWindow") << "GLFW error code: " << code << " " << message;
		}
	}
}


void CX::Private::App::setup (void) {

	ofSetWorkingDirectoryToDefault();

	setupWindow(CX::Private::CX_WindowConfiguration_t());

	//Why use these? CX has RNG and Clock.
	ofSeedRandom();
	ofResetElapsedTimeCounter();

	ofAddListener( ofEvents().exit, this, &CX::Private::App::exit, OF_EVENT_ORDER_APP );

	CX::Instances::Display.setup();

	CX::Instances::Log.captureOFLogMessages();
	CX::Instances::Log.levelForAllModules(CX_LogLevel::LOG_NOTICE);

	//Log.levelForConsole(CX_LogLevel::LOG_ALL);
	//Log.levelForFile(CX_LogLevel::LOG_ALL);
	//Log.levelForFile(CX_LogLevel::LOG_ALL, "Log for last run.txt");

	//CX::Private::Events.setup();

	//Call the user setup function
	setupExperiment();

	CX::Instances::Log.flush(); //Flush logs after the user setup function, so they can see if any errors happened during their setup.
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

void CX::Private::App::update (void) {
	updateExperiment(); //Call the user update function

	//ofSleepMillis(0); //sleep(0) is similar to yield()
}

void CX::Private::App::exit (ofEventArgs &a) {
	CX::Instances::Display.exit();

	ofRemoveListener( ofEvents().exit, this, &CX::Private::App::exit, OF_EVENT_ORDER_APP );
}


int main (void) {	
	CX::Private::App A;
	A.setup();
	while (true) {
		A.update();
	}
	return 0;
}