#include "CX_EntryPoint.h"

#include "CX_Private.h"

#include "CX_AppWindow.h"

/*! An instance of CX::CX_Display that is lightly hooked into the CX backend. setup() is called for Display before runExperiment() is called.
\ingroup entryPoint */
CX::CX_Display CX::Instances::Display;

/*! An instance of CX_InputManager that is very lightly hooked into the CX backend.
\ingroup entryPoint */
CX::CX_InputManager CX::Instances::Input; 

namespace CX {
	namespace Private {
		
		class App {
		public:

			App(void) :
				glVersionKnown(false)
			{}

			void setup(void);
			void setupWindow(CX_WindowConfiguration_t config);

			void learnOpenGLVersion(void);
			bool glVersionKnown;
			CX_GLVersion glVersion;
		};
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

	if (CX::Private::glfwContext == glfwGetCurrentContext()) {
		glfwDestroyWindow(CX::Private::glfwContext);
	}

	CX_GLVersion tempGLVersion;
	if (config.desiredOpenGLVersion.major != 0) {
		tempGLVersion = config.desiredOpenGLVersion;
	} else {
		learnOpenGLVersion(); //Must come before setupWindow.
		tempGLVersion = glVersion;
	}

	CX::Private::window = ofPtr<CX_AppWindow>(new CX_AppWindow);
	window->setOpenGLVersion(tempGLVersion.major, tempGLVersion.minor);
	window->setNumSamples(CX::Util::getSampleCount());

	
	if (config.desiredRenderer) {
		if (config.desiredRenderer->getType() == ofGLProgrammableRenderer::TYPE) {
			if (tempGLVersion.major >= 3 && tempGLVersion.minor >= 2) {
				ofSetCurrentRenderer(config.desiredRenderer, true);
			} else {
				CX::Instances::Log.warning() << "Desired renderer could not be used: The required OpenGL version is not available. Falling back on ofGLRenderer.";
				ofSetCurrentRenderer(ofPtr<ofBaseRenderer>(new ofGLRenderer), true);
			}
		} else {
			ofSetCurrentRenderer(config.desiredRenderer, true);
		}
	} else {
		/*
		//Check to see if the OpenGL version is high enough to fully support ofGLProgrammableRenderer. If not, fall back on ofGLRenderer.
		if (glver.major >= 3 && glver.minor >= 2) {
			ofSetCurrentRenderer(ofPtr<ofBaseRenderer>(new ofGLProgrammableRenderer), true);
		} else {
			ofSetCurrentRenderer(ofPtr<ofBaseRenderer>(new ofGLRenderer), true);
		}
		*/
		ofSetCurrentRenderer(ofPtr<ofBaseRenderer>(new ofGLRenderer), true);
	}


	ofSetupOpenGL(ofPtr<ofAppBaseWindow>(window), config.width, config.height, config.mode);

	ofGetCurrentRenderer()->update(); //Only needed for ofGLRenderer, not for ofGLProgrammableRenderer, but there is no harm in calling it

	Log.flush();

	window->initializeWindow();
	window->setWindowTitle("CX Experiment");

	CX::Private::glfwContext = glfwGetCurrentContext();

	//For some reason, this is needed in order to get text to display properly: (not any more, apparently)
	//ofSetOrientation(ofOrientation::OF_ORIENTATION_DEFAULT, true); 
}

//This function should only be called once
void CX::Private::App::learnOpenGLVersion(void) {

	if (glVersionKnown) {
		return;
	}

	//Find out what version of openGL the graphics card supports, which requires the creation 
	//of a GLFW window (or other initialization of openGL).

	glfwInit();
	GLFWwindow *windowP;
	glfwWindowHint(GLFW_VISIBLE, GL_FALSE); //Make the window invisible
	windowP = glfwCreateWindow(1, 1, "", NULL, NULL); //Create the window
	glfwMakeContextCurrent(windowP);
	this->glVersion = CX::Private::getOpenGLVersion(); //Once GL is initialized, get the version number
	glfwDestroyWindow(windowP);
	glfwWindowHint(GLFW_VISIBLE, GL_TRUE); //Make the next created window visible

	glVersionKnown = true;
}

CX::Private::App A;

void CX::relaunchWindow(CX_WindowConfiguration_t config) {
	A.setupWindow(config);
}

int main (void) {
	A.setup();
	runExperiment();
	return 0;
}