#include "CX_EntryPoint.h"

#include "CX_Private.h"

#include "CX_AppWindow.h"
#include "ofAppGLFWWindow.h"


namespace CX {
namespace Private {

void setupCX(void) {

	ofSetWorkingDirectoryToDefault();
	ofSetEscapeQuitsApp(false);

	CX::Instances::Log.captureOFLogMessages();
	CX::Instances::Log.levelForAllModules(CX_Logger::Level::LOG_ALL);
	CX::Instances::Log.level(CX_Logger::Level::LOG_NOTICE, "ofShader"); //Try to eliminate some of the shader gobbeldygook.


	CX::Private::learnOpenGLVersion(); //Should come before reopenWindow.

	bool openedSucessfully = reopenWindow(CX::CX_WindowConfiguration_t()); //or for the first time.

	if (openedSucessfully) {
		CX::Instances::Input.pollEvents(); //Do this so that the window is at least minimally responding and doesn't get killed by the OS.
			//This must happen after the window is configured because it relies on GLFW.

		CX::Instances::Disp.setup();
		CX::Instances::Disp.useHardwareVSync(true);
		CX::Instances::Disp.estimateFramePeriod(500);

		CX::Instances::Log.notice("CX_EntryPoint") << "Estimated frame period: " << CX::Instances::Disp.getFramePeriod() << " ms.";

		Clock.precisionTest(100000);
		
	} else {
		CX::Instances::Log.error("CX_EntryPoint") << "The window was not opened successfully.";
	}

	CX::Instances::Log.verbose() << endl << endl << "### End of startup logging data ###" << endl << endl;
	CX::Instances::Log.flush(); //Flush logs after setup, so user can see if any errors happened during setup.

	CX::Instances::Log.levelForAllModules(CX_Logger::Level::LOG_NOTICE);
	CX::Instances::Log.level(CX_Logger::Level::LOG_WARNING, "ofFbo"); //It isn't clear that this should be here, but the fbos
		//are really verbose when allocated and it is a lot of gibberish.
}

void reopenWindow080(CX_WindowConfiguration_t config) {

	CX::Private::appWindow = ofPtr<ofAppBaseWindow>(new CX::Private::CX_AppWindow);

	CX::Private::CX_AppWindow* awp = (CX::Private::CX_AppWindow*)CX::Private::appWindow.get();
	awp->setOpenGLVersion(config.desiredOpenGLVersion.major, config.desiredOpenGLVersion.minor);
	awp->setNumSamples(Util::getMsaaSampleCount());

	ofSetupOpenGL(CX::Private::appWindow, config.width, config.height, config.mode);
}

void reopenWindow084(CX_WindowConfiguration_t config) {

	/*
	Note that this section of code is a nasty hack that is only done because of a bug in openFrameworks.
	They are working on the bug, but in the mean time, I want to work around the bug to use new features of
	openFrameworks. The bug is that the pointer passed to ofSetupOpenGL is treated as an ofAppGLFWWindow
	regardless of whether it is one or not. Because CX_AppWindow is not ofAppGLFWWindow (although it's very close)
	passing a pointer to a CX_AppWindow results in, AFAIK, undefined behavior (and we don't want that, do we?).
	
	The hack: Allocate on a pointer enough memory to store either an ofAppGLFWWindow or a CX_AppWindow.
	Use placement new to put an ofAppGLFWWindow at that location.
	Pass that pointer to ofSetupOpenGL. It's an ofAppGLFWWindow, so no problem.
	The location pointed to by the pointer is now stored in the variable "window" in ofAppRunner.cpp.
	Now that the pointer is stored by the "window" variable, destroy the just-opened window.
	Finally, use placement new to create a CX_AppWindow where the pointer points to.
	Success!!!
	*/
	unsigned int appWindowAllocationSize = std::max(sizeof(CX::Private::CX_AppWindow), sizeof(ofAppGLFWWindow));

	void* windowP = new char[appWindowAllocationSize];
	windowP = new(windowP) ofAppGLFWWindow;

	glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
	ofSetupOpenGL((ofAppGLFWWindow*)windowP, config.width, config.height, config.mode);
	if (glfwGetCurrentContext() != NULL) {
		glfwDestroyWindow(glfwGetCurrentContext()); //Close temporary window
	}
	glfwWindowHint(GLFW_VISIBLE, GL_TRUE);

	windowP = new(windowP) CX::Private::CX_AppWindow;
	////////////////////
	// End nasty hack //
	////////////////////

	CX::Private::appWindow = ofPtr<ofAppBaseWindow>((CX::Private::CX_AppWindow*)windowP);

	CX::Private::CX_AppWindow* awp = (CX::Private::CX_AppWindow*)CX::Private::appWindow.get();
	awp->setOpenGLVersion(config.desiredOpenGLVersion.major, config.desiredOpenGLVersion.minor);
	awp->setNumSamples(Util::getMsaaSampleCount());

	((CX::Private::CX_AppWindow*)CX::Private::appWindow.get())->setupOpenGL(config.width, config.height, config.mode, config.preOpeningUserFunction);
}

} //namespace Private

/*! This function opens a GLFW window that can be rendered to. If another window was already
open by the application at the time this is called, that window will be closed. This is useful
if you want to control some of the parameters of the window that cannot be changed after the window
has been opened.
\param config Configuration options for the window to be opened.
\return `true` if reopening the window was successful, `false` otherwise.
*/
bool reopenWindow(CX_WindowConfiguration_t config) {
	if (CX::Private::glfwContext == glfwGetCurrentContext()) {
		glfwDestroyWindow(CX::Private::glfwContext); //Close previous window
		CX::Private::glfwContext = NULL;
	}

	Private::setMsaaSampleCount(config.msaaSampleCount);

	if (config.desiredOpenGLVersion.major <= 0) {
		config.desiredOpenGLVersion = Private::getOpenGLVersion();
	}

	if (config.desiredRenderer) {
		if (config.desiredRenderer->getType() == ofGLProgrammableRenderer::TYPE) {
			if (Private::glCompareVersions(config.desiredOpenGLVersion, Private::CX_GLVersion(3, 2, 0)) >= 0) {
				ofSetCurrentRenderer(config.desiredRenderer, true);
			} else {
				CX::Instances::Log.warning("CX_EntryPoint") << "Desired renderer could not be used: "
					"High enough version of OpenGL is not available (requires OpenGL >= 3.2). Falling back on ofGLRenderer.";
				ofSetCurrentRenderer(ofPtr<ofBaseRenderer>(new ofGLRenderer), true);
			}
		} else {
			ofSetCurrentRenderer(config.desiredRenderer, true);
		}
	} else {
		//Check to see if the OpenGL version is high enough to fully support ofGLProgrammableRenderer. If not, fall back on ofGLRenderer.
		if (Private::glCompareVersions(config.desiredOpenGLVersion, Private::CX_GLVersion(3, 2, 0)) >= 0) {
			ofSetCurrentRenderer(ofPtr<ofBaseRenderer>(new ofGLProgrammableRenderer), true);
		} else {
			ofSetCurrentRenderer(ofPtr<ofBaseRenderer>(new ofGLRenderer), true);
		}
	}

	try {
		if (CX::Util::checkOFVersion(0, 8, 0, false)) {
			CX::Private::reopenWindow080(config);
		} else if (CX::Util::checkOFVersion(0, 8, 4, false)) {
			CX::Private::reopenWindow084(config);
		} else {
			CX::Instances::Log.error("CX_EntryPoint") << "reopenWindow(): The current version of openFrameworks is not supported by CX. "
				"Version 0.8.4 of openFrameworks is recommended.";
			return false;
		}
		CX::Private::glfwContext = glfwGetCurrentContext();
	} catch (std::exception& e) {
		CX::Instances::Log.error("CX_EntryPoint") << "reopenWindow(): Exception caught while setting up window: " << e.what();
	} catch (...) {
		CX::Instances::Log.error("CX_EntryPoint") << "reopenWindow(): Exception caught while setting up window.";
	}

	if (CX::Private::glfwContext == NULL) {
		CX::Instances::Log.error("CX_EntryPoint") << "reopenWindow(): There was an error setting up the window.";
		return false;
	}

	ofGetCurrentRenderer()->update(); //Only needed for ofGLRenderer, not for ofGLProgrammableRenderer, but there is no harm in calling it

	CX::Private::appWindow->initializeWindow();
	CX::Private::appWindow->setWindowTitle(config.windowTitle);

	return true;
}

} //namespace CX


int main (void) {
	CX::Private::setupCX();

	runExperiment();

	CX::Instances::Log.flush();
	return 0;
}
