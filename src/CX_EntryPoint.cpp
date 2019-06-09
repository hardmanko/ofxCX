#include "CX_EntryPoint.h"

#include "CX_Private.h"

#include "CX_AppWindow.h" // 0.8.4 remove
#include "ofAppGLFWWindow.h"

#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR >= 9 && OF_VERSION_PATCH >= 0
#include "ofAppRunner.h"
#endif


namespace CX {

void setupKeyboardShortcuts(void) {

	auto toggleFullscreen = [](void) {
		Instances::Disp.setFullscreen(!Instances::Disp.isFullscreen());
	};

	Instances::Input.Keyboard.addShortcut("Toggle fullscreen: LEFT_ALT + F1",
		{ Keycode::LEFT_ALT, Keycode::F1 },
		toggleFullscreen
	);

}

/*! This function initializes CX functionality. It should probably only be called once, at program start.
\param config The intial CX configuration.
\return `true` if intialization was successful, `false` if there was an error. If there was an error, it should be logged.
*/
bool initializeCX(CX_InitConfiguation config) {

	if (config.clockPrecisionTestIterations < 10000) {
		config.clockPrecisionTestIterations = 10000;
	}

#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR >= 9 && OF_VERSION_PATCH >= 0
	ofInit();
#else //Older versions...
	ofSetWorkingDirectoryToDefault();
#endif
	
	ofSetEscapeQuitsApp(false);

	// Set up the clock
	CX::Instances::Clock.setup(nullptr, true, config.clockPrecisionTestIterations);

	CX::Instances::Log.captureOFLogMessages(config.captureOFLogMessages);
	CX::Instances::Log.levelForAllModules(CX_Logger::Level::LOG_ALL);
	CX::Instances::Log.levelForModule(CX_Logger::Level::LOG_NOTICE, "ofShader"); //Try to eliminate some of the verbose shader gobbeldygook.

	CX::Private::learnOpenGLVersion(); //Should come before reopenWindow.

	bool openedSucessfully = CX::reopenWindow(config.windowConfig); //or for the first time.

	if (!openedSucessfully) {
		CX::Instances::Log.error("CX_EntryPoint") << "initializeCX(): The window was not opened successfully.";
	} else {

		CX::Instances::Input.pollEvents(); //Do this so that the window is at least minimally responding and doesn't get killed by the OS.
			//This must happen after the window is configured because it relies on GLFW.

		if (config.framePeriodEstimationInterval != CX_Millis(0)) {
			CX::Instances::Disp.estimateFramePeriod(config.framePeriodEstimationInterval);
			CX::Instances::Log.notice("CX_EntryPoint") << "initializeCX(): Estimated frame period for display: " << CX::Instances::Disp.getFramePeriod().millis() << " ms.";
		}

		setupKeyboardShortcuts();


		// Set up sound
		CX::Instances::SoundPlayer.setup(&CX::Instances::SoundStream);
		CX::Instances::SoundRecorder.setup(&CX::Instances::SoundStream);


		//This is temporary: I think there's an oF bug about it
#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 8
		glfwSetWindowPos(CX::Private::glfwContext, 200, 200);
#endif
		
	}

	CX::Instances::Log.verbose() << std::endl << std::endl << "### End of startup logging data ###" << std::endl << std::endl;
	CX::Instances::Log.flush(); // Flush logs after setup so user can see if any errors happened during setup.

	// By default, suppress verbose messages to the console
	CX::Instances::Log.levelForConsole(CX_Logger::Level::LOG_NOTICE);

	// It isn't clear that this should be here, but the fbos
	// are really verbose (with notices) when allocated and it is a lot of gibberish.
	CX::Instances::Log.levelForModule(CX_Logger::Level::LOG_WARNING, "ofFbo"); 

	return openedSucessfully;
}

bool terminateCX(void) {

	CX::Instances::Log.flush();

	//glfwTerminate();

	return true;
}

namespace Private {

void setDesiredRenderer(const CX_WindowConfiguration& config, bool setDefaultRendererIfNoneDesired, ofAppBaseWindow* window) {
	if (config.desiredRenderer != nullptr) {
		if (config.desiredRenderer->getType() == ofGLProgrammableRenderer::TYPE) {
			if (Private::glCompareVersions(config.desiredOpenGLVersion, Private::CX_GLVersion(3, 2, 0)) >= 0) {
				ofSetCurrentRenderer(config.desiredRenderer, true);
			} else {
				CX::Instances::Log.warning("CX_EntryPoint") << "Desired renderer could not be used: "
					"High enough version of OpenGL is not available (requires OpenGL >= 3.2). Falling back on ofGLRenderer.";

#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR >= 9 && OF_VERSION_PATCH >= 0
				std::shared_ptr<ofBaseRenderer> newRenderer = std::shared_ptr<ofBaseRenderer>(new ofGLRenderer(window));
				std::dynamic_pointer_cast<ofGLRenderer>(newRenderer)->setup();
				ofSetCurrentRenderer(newRenderer, true);
				window->renderer() = newRenderer; //This seems to be unnecessary as of 0.10.1
#else
				ofSetCurrentRenderer(std::shared_ptr<ofBaseRenderer>(new ofGLRenderer), true);
#endif

			}
		} else {
			ofSetCurrentRenderer(config.desiredRenderer, true);
		}

		return;

	}

	if (!setDefaultRendererIfNoneDesired) {
		return;
	}

	//Check to see if the OpenGL version is high enough to fully support ofGLProgrammableRenderer. If not, fall back on ofGLRenderer.
	if (Private::glCompareVersions(config.desiredOpenGLVersion, Private::CX_GLVersion(3, 2, 0)) >= 0) {

#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR >= 9 && OF_VERSION_PATCH >= 0
		std::shared_ptr<ofBaseRenderer> newRenderer = std::shared_ptr<ofBaseRenderer>(new ofGLProgrammableRenderer(window));
		std::dynamic_pointer_cast<ofGLProgrammableRenderer>(newRenderer)->setup(config.desiredOpenGLVersion.major, config.desiredOpenGLVersion.minor);
		ofSetCurrentRenderer(newRenderer, true);
		window->renderer() = newRenderer;
#else
		ofSetCurrentRenderer(std::shared_ptr<ofBaseRenderer>(new ofGLProgrammableRenderer), true);
#endif

	} else {

#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR >= 9 && OF_VERSION_PATCH >= 0
		std::shared_ptr<ofBaseRenderer> newRenderer = std::shared_ptr<ofBaseRenderer>(new ofGLRenderer(window));
		std::dynamic_pointer_cast<ofGLRenderer>(newRenderer)->setup();
		ofSetCurrentRenderer(newRenderer, true);
		window->renderer() = newRenderer;
#else
		ofSetCurrentRenderer(std::shared_ptr<ofBaseRenderer>(new ofGLRenderer), true);
#endif

	}

	
}

#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR >= 9 && OF_VERSION_PATCH >= 0
bool exitCallbackHandler(ofEventArgs& args) {

	ofNotifyEvent(CX::Private::getEvents().exitEvent);

#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 10 && OF_VERSION_PATCH == 0
	glfwTerminate(); // This is not supposed to be called from callbacks
	// but without calling this, the window hangs on 0.10.0.
	// Not needed on 0.10.1.
#endif

	return true;
}
#endif


#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR >= 10 && OF_VERSION_PATCH >= 0

void reopenWindow_0_10_0(CX_WindowConfiguration config) {

	bool firstCall = (CX::Private::appWindow == nullptr);

	if (firstCall) {
		CX::Private::appWindow = std::shared_ptr<ofAppBaseWindow>(new CX::Private::CX_AppWindow);
	} else {
		CX::Private::appWindow->close();
	}

	std::shared_ptr<CX::Private::CX_AppWindow> awp = std::dynamic_pointer_cast<CX::Private::CX_AppWindow>(CX::Private::appWindow);

	ofGLFWWindowSettings settings;
	settings.windowMode = config.mode;
	settings.glVersionMajor = config.desiredOpenGLVersion.major;
	settings.glVersionMinor = config.desiredOpenGLVersion.minor;
	settings.numSamples = config.msaaSampleCount;
	settings.resizable = config.resizeable;
	settings.setSize(config.width, config.height);

	awp->setup(settings);

	awp->events().enable();

	if (firstCall) {
		ofGetMainLoop()->addWindow(awp);
	}

	if (awp->getGLFWWindow() != nullptr) {

		setDesiredRenderer(config, true, CX::Private::appWindow.get());

		ofAddListener(ofEvents().exit, &exitCallbackHandler, ofEventOrder::OF_EVENT_ORDER_AFTER_APP);

		//ofAppGLFWWindow doesn't show the window until the first call to update()
		glfwShowWindow(glfwGetCurrentContext());
	}
}

#elif OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 9 && OF_VERSION_PATCH >= 0

void reopenWindow090(CX_WindowConfiguration config) {

	bool firstCall = (CX::Private::appWindow == nullptr);

	if (firstCall) {
		CX::Private::appWindow = shared_ptr<ofAppBaseWindow>(new CX::Private::CX_AppWindow);
	} else {
		CX::Private::appWindow->close();
	}

	std::shared_ptr<CX::Private::CX_AppWindow> awp = std::dynamic_pointer_cast<CX::Private::CX_AppWindow>(CX::Private::appWindow);

	ofGLFWWindowSettings settings;
	settings.windowMode = config.mode;
	settings.glVersionMajor = config.desiredOpenGLVersion.major;
	settings.glVersionMinor = config.desiredOpenGLVersion.minor;
	settings.numSamples = config.msaaSampleCount;
	settings.resizable = config.resizeable;
	settings.width = config.width;
	settings.height = config.height;

	awp->setup(settings);

	awp->events().enable();
	
	if (firstCall) {
		ofGetMainLoop()->addWindow(awp);
	}

	if (awp->getGLFWWindow() != nullptr) {

		setDesiredRenderer(config, true, CX::Private::appWindow.get());

		ofAddListener(ofEvents().exit, &exitCallbackHandler, ofEventOrder::OF_EVENT_ORDER_AFTER_APP);

		//ofAppGLFWWindow doesn't show the window until the first call to update()
		glfwShowWindow(glfwGetCurrentContext());
	}
}

#else

void reopenWindow080(CX_WindowConfiguration config) {

	//Close previous window, if opened
	if (CX::Private::glfwContext == glfwGetCurrentContext()) {
		glfwDestroyWindow(CX::Private::glfwContext);
		CX::Private::glfwContext = nullptr;
	}

	CX::Private::setDesiredRenderer(config, true, nullptr);



	CX::Private::appWindow = std::shared_ptr<ofAppBaseWindow>(new CX::Private::CX_AppWindow);

	CX::Private::CX_AppWindow* awp = (CX::Private::CX_AppWindow*)CX::Private::appWindow.get();
	awp->setOpenGLVersion(config.desiredOpenGLVersion.major, config.desiredOpenGLVersion.minor);
	awp->setNumSamples(Util::getMsaaSampleCount());

	ofSetupOpenGL(CX::Private::appWindow, config.width, config.height, config.mode);
}

void reopenWindow084(CX_WindowConfiguration config) {

	//Close previous window, if opened
	if (CX::Private::glfwContext == glfwGetCurrentContext()) {
		glfwDestroyWindow(CX::Private::glfwContext);
		CX::Private::glfwContext = nullptr;
	}

	CX::Private::setDesiredRenderer(config, true, nullptr);

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

	// shouldn't there be
	//delete windowP;
	// here?
	windowP = new(windowP) CX::Private::CX_AppWindow;
	////////////////////
	// End nasty hack //
	////////////////////

	CX::Private::appWindow = std::shared_ptr<ofAppBaseWindow>((CX::Private::CX_AppWindow*)windowP);

	CX::Private::CX_AppWindow* awp = (CX::Private::CX_AppWindow*)CX::Private::appWindow.get();
	awp->setOpenGLVersion(config.desiredOpenGLVersion.major, config.desiredOpenGLVersion.minor);
	awp->setNumSamples(Util::getMsaaSampleCount());

	((CX::Private::CX_AppWindow*)CX::Private::appWindow.get())->setupOpenGL(config.width, config.height, config.mode, config.preOpeningUserFunction, config.resizeable);
}

#endif

} //namespace Private

/*! This function opens a GLFW window that can be rendered to. If another window was already
open by the application at the time this is called, that window will be closed. This is useful
if you want to control some of the parameters of the window that cannot be changed after the window
has been opened.
\param config Configuration options for the window to be opened.
\return `true` if reopening the window was successful, `false` otherwise.
*/
bool reopenWindow(CX_WindowConfiguration config) {

	Private::setMsaaSampleCount(config.msaaSampleCount);

	if (config.desiredOpenGLVersion.major <= 0) {
		config.desiredOpenGLVersion = Private::getOpenGLVersion();
	}

	try {
#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 10 && OF_VERSION_PATCH >= 0
		CX::Private::reopenWindow_0_10_0(config);
#elif OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 9 && OF_VERSION_PATCH >= 0
		CX::Private::reopenWindow090(config);
#elif OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 8 && OF_VERSION_PATCH == 4
		CX::Private::reopenWindow084(config);
#elif OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 8 && OF_VERSION_PATCH == 0
		CX::Private::reopenWindow080(config);
#else
		CX::Instances::Log.error("CX_EntryPoint") << "reopenWindow(): The current version of openFrameworks is not supported by CX. "
			"Version 0.10.1 of openFrameworks is recommended.";
		return false;
#endif

	} catch (std::exception& e) {
		CX::Instances::Log.error("CX_EntryPoint") << "reopenWindow(): Exception caught while setting up window: " << e.what();
	} catch (...) {
		CX::Instances::Log.error("CX_EntryPoint") << "reopenWindow(): Unknown exception caught while setting up window.";
	}

	if (glfwGetCurrentContext() == nullptr) {
		CX::Instances::Log.error("CX_EntryPoint") << "reopenWindow(): There was an error setting up the window.";
		return false;
	}

	Private::glfwContext = glfwGetCurrentContext();
	Private::glfwContextManager.setup(glfwGetCurrentContext(), std::this_thread::get_id());

	//Setup the display for the new window
	CX::Instances::Disp.setup();

#if !(OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR >= 9 && OF_VERSION_PATCH >= 0)
	ofGetCurrentRenderer()->update(); //Only needed for ofGLRenderer, not for ofGLProgrammableRenderer, but there is no harm in calling it
	CX::Private::appWindow->initializeWindow();
#endif
	ofSetWindowTitle(config.windowTitle);

	return true;
}

} //namespace CX


#ifndef CX_NO_MAIN
int main(void) {
	CX::initializeCX(CX_InitConfiguation());

	CX::Instances::Clock.resetExperimentStartTime();

	// Always runExperiment even if initialization failed so user code can see that fact.
	runExperiment();

	CX::terminateCX();

	return 0;
}
#endif
