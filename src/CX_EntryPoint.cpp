#include "CX_EntryPoint.h"

#include "ofGLRenderer.h"
#include "ofGLProgrammableRenderer.h"
#include "ofAppGLFWWindow.h"
#include "ofAppRunner.h"

#include "CX_Display.h"
#include "CX_Logger.h"
#include "CX_InputManager.h"


namespace CX {

namespace Private {
	CX_State State;
}

namespace Util {

	/*! This function retrieves the MSAA (http://en.wikipedia.org/wiki/Multisample_anti-aliasing)
	sample count. The sample count can be set by calling CX::relaunchWindow() with the desired sample
	count set in the argument to relaunchWindow(). */
	unsigned int getMsaaSampleCount(void) {
		return CX::Private::State.initConfig.windowConfig.msaaSampleCount;
	};


	// Find out what version of openGL the graphics card supports, which requires the creation
	// of a GLFW window (or other initialization of openGL).
	void learnHighestOpenGLVersion(void) {

		if (glfwInit() != GLFW_TRUE) {
			return;
		}

		GLFWwindow *windowP;
		glfwWindowHint(GLFW_VISIBLE, GL_FALSE); //Make the window invisible
		windowP = glfwCreateWindow(1, 1, "", NULL, NULL); //Create the window
		glfwMakeContextCurrent(windowP);

		//Once GL is initialized, get the version number from the version number string.
		std::string s = (char*)glGetString(GL_VERSION);

		std::vector<std::string> versionVendor = ofSplitString(s, " "); //Vendor specific information follows a space, so split it off.
		std::vector<std::string> version = ofSplitString(versionVendor[0], "."); //Version numbers

		CX::Private::State.maxGLVersion.major = ofToInt(version[0]);
		CX::Private::State.maxGLVersion.minor = ofToInt(version[1]);
		CX::Private::State.maxGLVersion.release = (version.size() == 3) ? ofToInt(version[2]) : 0;

		glfwDestroyWindow(windowP);
		glfwWindowHint(GLFW_VISIBLE, GL_TRUE); //Make the next created window visible

	}

	CX_GLVersion getHighestOpenGLVersion(void) {
		return CX::Private::State.maxGLVersion;
	}

	/*! Checks that the version of openFrameworks that is used during compilation matches the requested version.

	If the desired oF version is 0.8.1, call `checkOFVersion(0, 8, 1)`.

	\param versionMajor The major version (the X in X.0.0).
	\param versionMinor The minor version (0.X.0).
	\param versionPatch The patch version (0.0.X).

	\param log If `true`, a version mismatch will result in a warning being logged.
	\return `true` if the versions match, `false` otherwise. */
	bool checkOFVersion(int versionMajor, int versionMinor, int versionPatch, bool log) {
		if (versionMajor == OF_VERSION_MAJOR && versionMinor == OF_VERSION_MINOR && versionPatch == OF_VERSION_PATCH) {
			return true;
		}
		if (log) {
			CX::Instances::Log.warning("CX::Util::checkOFVersion") <<
				"openFrameworks version does not match target version. Current oF version: " << ofGetVersionInfo();
		}
		return false;
	}

} // namespace Util

/*! Compare `CX_GLVersion`s.
\param that `CX_GLVersion` to compare to.
\return One of:
+ `this > that: 1`
+ `this == that: 0`
+ `this < that: -1`
*/
int CX_GLVersion::compare(int maj, int min, int rel) const {
	return compare(CX_GLVersion(maj, min, rel));
}

int CX_GLVersion::compare(const CX_GLVersion& that) const {

	if (this->major > that.major) {
		return 1;
	}
	else if (this->major < that.major) {
		return -1;
	}

	if (this->minor > that.minor) {
		return 1;
	}
	else if (this->minor < that.minor) {
		return -1;
	}

	if (this->release > that.release) {
		return 1;
	}
	else if (this->release < that.release) {
		return -1;
	}

	return 0;
}

/*! Fence Sync is supported by OpenGL version 3.2.0 and higher.

Fence sync is also supported by ARB_sync, but that means dealing 
with potentially device-specific implementations. Only proper
fence sync is supported, not ARB_sync.

\return `true` if this is at least 3.2.0.
*/
bool CX_GLVersion::supportsGLFenceSync(void) const {
	return this->compare(3, 2, 0) >= 0;
}

// See https://www.khronos.org/opengl/wiki/Core_Language_(GLSL)#OpenGL_and_GLSL_versions
// Also https://en.wikipedia.org/wiki/OpenGL_Shading_Language#Versions
CX_GLVersion CX_GLVersion::getCorrespondingGLSLVersion(void) const {
	if (this->major < 2) {
		return CX_GLVersion(0, 0, 0); //No version exists
	}
	else if (this->major == 2 && this->minor == 0) {
		return CX_GLVersion(1, 10, 59);
	}
	else if (this->major == 2 && this->minor == 1) {
		return CX_GLVersion(1, 20, 8);
	}
	else if (this->major == 3 && this->minor == 0) {
		return CX_GLVersion(1, 30, 10);
	}
	else if (this->major == 3 && this->minor == 1) {
		return CX_GLVersion(1, 40, 8);
	}
	else if (this->major == 3 && this->minor == 2) {
		return CX_GLVersion(1, 50, 11);
	}
	else if (this->compare(3, 3, 0) >= 0) {
		return *this;
	}

	return CX_GLVersion(0, 0, 0); //No version exists
}



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

	config.clockPrecisionTestIterations = std::max<unsigned int>(config.clockPrecisionTestIterations, 10000);

	CX::Private::State.initConfig = config;

	ofInit();

	ofSetEscapeQuitsApp(false);

	// Set up the clock
	CX::Instances::Clock.setup(nullptr, true, config.clockPrecisionTestIterations);

	// Set up logging
	CX::Instances::Log.captureOFLogMessages(config.captureOFLogMessages);
	CX::Instances::Log.levelForAllModules(CX_Logger::Level::LOG_ALL);
	CX::Instances::Log.levelForModule(CX_Logger::Level::LOG_NOTICE, "ofShader"); //Try to eliminate some of the verbose shader gobbeldygook.

	CX::Util::learnHighestOpenGLVersion(); //Should come before reopenWindow.

	bool openedSucessfully = CX::reopenWindow(config.windowConfig); //or for the first time.

	if (!openedSucessfully) {
		CX::Instances::Log.error("CX_EntryPoint") << "initializeCX(): The window was not opened successfully.";
	} else {

		CX::Instances::Input.pollEvents(); //Do this so that the window is at least minimally responding and doesn't get killed by the OS.
			//This must happen after the window is configured because it relies on GLFW.

		if (config.framePeriodEstimationInterval != CX_Millis(0)) {
			CX::Instances::Disp.estimateFramePeriod(config.framePeriodEstimationInterval);
			CX::Instances::Disp.setFramePeriod(CX::Instances::Disp.getFramePeriod(), true);
			CX::Instances::Log.notice("CX_EntryPoint") << "initializeCX(): Estimated frame period for display: " << CX::Instances::Disp.getFramePeriod().millis() << " ms.";
		}

		setupKeyboardShortcuts();
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
	//ofExit(); // ?

	return true;
}

namespace Private {

void setupGLRenderer(ofAppBaseWindow* window) {

	std::shared_ptr<ofBaseRenderer> newRenderer = std::shared_ptr<ofBaseRenderer>(new ofGLRenderer(window));

	std::dynamic_pointer_cast<ofGLRenderer>(newRenderer)->setup();
	ofSetCurrentRenderer(newRenderer, true);

	window->renderer() = newRenderer; //This seems to be unnecessary as of 0.10.1
}

void setDesiredRenderer(const CX_WindowConfiguration& config, bool setDefaultRendererIfNoneDesired, ofAppBaseWindow* window) {

	bool desiredGlVerAtLeast320 = config.desiredOpenGLVersion.compare(3, 2, 0) >= 0;

	if (config.desiredRenderer != nullptr) {
		if (config.desiredRenderer->getType() == ofGLProgrammableRenderer::TYPE) {

			if (desiredGlVerAtLeast320) {
				ofSetCurrentRenderer(config.desiredRenderer, true);
			} else {
				CX::Instances::Log.warning("CX_EntryPoint") << "Desired renderer could not be used: "
					"High enough version of OpenGL is not available (requires OpenGL >= 3.2). Falling back on ofGLRenderer.";

				setupGLRenderer(window);

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
	if (desiredGlVerAtLeast320) {

		std::shared_ptr<ofBaseRenderer> newRenderer = std::shared_ptr<ofBaseRenderer>(new ofGLProgrammableRenderer(window));
		std::dynamic_pointer_cast<ofGLProgrammableRenderer>(newRenderer)->setup(config.desiredOpenGLVersion.major, config.desiredOpenGLVersion.minor);
		ofSetCurrentRenderer(newRenderer, true);
		window->renderer() = newRenderer; //This seems to be unnecessary as of 0.10.1

	} else {
		setupGLRenderer(window);
	}
}


bool exitCallbackHandler(ofEventArgs& args) {

	ofNotifyEvent(CX::Private::getEvents().exitEvent);

	terminateCX(); // maybe have extra argument inExitCallback to terminateCX?

#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 10 && OF_VERSION_PATCH == 0
	glfwTerminate(); // This is not supposed to be called from callbacks
	// but without calling this, the window hangs on 0.10.0.
	// Not needed on 0.10.1.
#endif

	return true;
}

#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR >= 10 && OF_VERSION_PATCH >= 0

void reopenWindow_0_10_0(CX_WindowConfiguration config) {

	bool firstCall = (CX::Private::State.appWindow == nullptr);

	if (firstCall) {
		CX::Private::State.appWindow = std::shared_ptr<ofAppBaseWindow>(new ofAppGLFWWindow);
	} else {
		CX::Private::State.appWindow->close();
	}

	std::shared_ptr<ofAppGLFWWindow> awp = std::dynamic_pointer_cast<ofAppGLFWWindow>(CX::Private::State.appWindow);

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

		setDesiredRenderer(config, true, CX::Private::State.appWindow.get());

		ofAddListener(ofEvents().exit, &exitCallbackHandler, ofEventOrder::OF_EVENT_ORDER_AFTER_APP);

		//ofAppGLFWWindow doesn't show the window until the first call to update()
		glfwShowWindow(glfwGetCurrentContext());
	}
}

#elif OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 9 && OF_VERSION_PATCH >= 0

void reopenWindow_0_9_0(CX_WindowConfiguration config) {

	bool firstCall = (CX::Private::appWindow == nullptr);

	if (firstCall) {
		CX::Private::appWindow = shared_ptr<ofAppBaseWindow>(new ofAppGLFWWindow);
	} else {
		CX::Private::appWindow->close();
	}

	std::shared_ptr<ofAppGLFWWindow> awp = std::dynamic_pointer_cast<ofAppGLFWWindow>(CX::Private::appWindow);

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

	if (config.desiredOpenGLVersion.major <= 0) {
		config.desiredOpenGLVersion = CX::Util::getHighestOpenGLVersion();
	}

	try {
#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 10 && OF_VERSION_PATCH >= 0
		CX::Private::reopenWindow_0_10_0(config);
#elif OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 9 && OF_VERSION_PATCH >= 0
		CX::Private::reopenWindow_0_9_0(config);
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

	CX::Private::State.glfwContextManager.setup(glfwGetCurrentContext(), std::this_thread::get_id());

	//Setup the display for the new window
	CX::Instances::Disp.setup();

	ofSetWindowTitle(config.windowTitle);

	return true;
}

} //namespace CX


#ifndef CX_NO_MAIN
int main(void) {
	CX::initializeCX(CX::CX_InitConfiguation());

	CX::Instances::Clock.resetExperimentStartTime();

	// Always runExperiment even if initialization failed so user code can see that fact.
	runExperiment();

	CX::terminateCX();

	return 0;
}
#endif
