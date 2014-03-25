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
		void setupCX(void);
	}
}


void CX::Private::setupCX(void) {

	ofSetWorkingDirectoryToDefault();

	CX::Instances::Log.captureOFLogMessages();
	CX::Instances::Log.levelForAllModules(CX_LogLevel::LOG_ALL);

	Util::checkOFVersion(0, 8, 0); //Check to make sure that the version of oF that is being used is supported by CX.

	Private::learnOpenGLVersion(); //Should come before relaunchWindow.
	relaunchWindow(CX::CX_WindowConfiguration_t()); //or for the first time.

	CX::Instances::Input.pollEvents(); //So that the window is at least minimally responding
		//This must happen after the window is configured because it relies on GLFW.

	//Why use these? CX has RNG and Clock.
	ofSeedRandom();
	ofResetElapsedTimeCounter();

	CX::Instances::Display.setup();

	Clock.precisionTest(10000);

	CX::Instances::Log.flush(); //Flush logs after setup, so user can see if any errors happened during setup.
	CX::Instances::Log.levelForAllModules(CX_LogLevel::LOG_NOTICE);
}


void CX::relaunchWindow(CX_WindowConfiguration_t config) {
	if (CX::Private::glfwContext == glfwGetCurrentContext()) {
		glfwDestroyWindow(CX::Private::glfwContext); //Close previous window
	}

	Private::CX_GLVersion tempGLVersion;
	if (config.desiredOpenGLVersion.major > 0) {
		tempGLVersion = config.desiredOpenGLVersion;
	} else {
		tempGLVersion = Private::getOpenGLVersion();
	}

	Private::setSampleCount(config.multisampleSampleCount);

	Private::window = ofPtr<Private::CX_AppWindow>(new Private::CX_AppWindow);
	Private::window->setOpenGLVersion(tempGLVersion.major, tempGLVersion.minor);
	Private::window->setNumSamples(config.multisampleSampleCount);


	if (config.desiredRenderer) {
		if (config.desiredRenderer->getType() == ofGLProgrammableRenderer::TYPE) {
			if (Private::glCompareVersions(tempGLVersion, Private::CX_GLVersion(3,2,0)) == 1) {
				ofSetCurrentRenderer(config.desiredRenderer, true);
			} else {
				CX::Instances::Log.warning() << "Desired renderer could not be used: The required OpenGL version is not available. Falling back on ofGLRenderer.";
				ofSetCurrentRenderer(ofPtr<ofBaseRenderer>(new ofGLRenderer), true);
			}
		} else {
			ofSetCurrentRenderer(config.desiredRenderer, true);
		}
	} else {
		
		//Check to see if the OpenGL version is high enough to fully support ofGLProgrammableRenderer. If not, fall back on ofGLRenderer.
		if (Private::glCompareVersions(tempGLVersion, Private::CX_GLVersion(3,2,0)) == 1) {
			ofSetCurrentRenderer(ofPtr<ofBaseRenderer>(new ofGLProgrammableRenderer), true);
		} else {
			ofSetCurrentRenderer(ofPtr<ofBaseRenderer>(new ofGLRenderer), true);
		}
		
		//ofSetCurrentRenderer(ofPtr<ofBaseRenderer>(new ofGLRenderer), true);
	}

	ofSetupOpenGL(ofPtr<ofAppBaseWindow>(Private::window), config.width, config.height, config.mode);

	ofGetCurrentRenderer()->update(); //Only needed for ofGLRenderer, not for ofGLProgrammableRenderer, but there is no harm in calling it

	Instances::Log.flush();

	Private::window->initializeWindow();
	Private::window->setWindowTitle("CX Experiment");

	Private::glfwContext = glfwGetCurrentContext();
}



int main (void) {
	CX::Private::setupCX();
	runExperiment();
	return 0;
}