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

	std::shared_ptr<CX_GlobalState> globalStateFactory(void) {
		return std::shared_ptr<CX_GlobalState>(new CX_GlobalState());
	}

	std::shared_ptr<CX_GlobalState> State;

	void CX_GlobalState::setCXIntialization(bool cxInitialized) {
		_cxIsInitialized = cxInitialized;
		if (cxInitialized) {
			_firstInitialization = false;
		}
	}

	bool CX_GlobalState::hasCXBeenInitialized(void) const {
		return _firstInitialization;
	}

	bool CX_GlobalState::isCXInitialized(void) const {
		return _cxIsInitialized;
	}

	void CX_GlobalState::setInitConfig(const CX_InitConfiguation& cfg) {
		_initConfig = cfg;
	}

	const CX_InitConfiguation& CX_GlobalState::getInitConfig(void) const {
		return _initConfig;
	}

	/*! This function retrieves the MSAA (http://en.wikipedia.org/wiki/Multisample_anti-aliasing)
	sample count. The sample count can be set by calling CX::relaunchWindow() with the desired sample
	count set in the argument to relaunchWindow(). */
	unsigned int CX_GlobalState::getMsaaSampleCount(void) const {
		return _initConfig.windowConfig.msaaSampleCount;
	}

	//bool CX_GlobalState::cxIsInitialized(void) const {
	//	return false;
	//}

	void CX_GlobalState::setAppWindow(std::shared_ptr<ofAppBaseWindow> wind) {
		_appWindow = wind;
	}

	std::shared_ptr<ofAppBaseWindow> CX_GlobalState::getAppWindow(void) const {
		return _appWindow;
	}

	const Util::GLVersion& CX_GlobalState::getHighestOpenGLVersion(void) const {
		if (!_glVersionLearned) {
			// warn?
		}
		return _maxGLVersion;
	}

	/*! Learn what version of OpenGL is supported by your system.
	This function is called during CX initialization.
	Can be accessed with CX_GlobalState::getHighestOpenGLVersion(). */
	bool CX_GlobalState::learnHighestOpenGLVersion(void) {

		this->_maxGLVersion = Util::GLVersion(); // reset
		this->_glVersionLearned = false;

		if (glfwInit() != GLFW_TRUE) {
			return false;
		}

		GLFWwindow *windowP;
		glfwWindowHint(GLFW_VISIBLE, GL_FALSE); //Make the window invisible
		windowP = glfwCreateWindow(1, 1, "", NULL, NULL); //Create the window
		glfwMakeContextCurrent(windowP);

		//Once GL is initialized, get the version number from the version number string.
		std::string s = (char*)glGetString(GL_VERSION);

		std::vector<std::string> versionVendor = ofSplitString(s, " "); //Vendor specific information follows a space, so split it off.
		std::vector<std::string> version = ofSplitString(versionVendor[0], "."); //Version numbers

		this->_maxGLVersion.major = ofToInt(version[0]);
		this->_maxGLVersion.minor = ofToInt(version[1]);
		this->_maxGLVersion.release = (version.size() == 3) ? ofToInt(version[2]) : 0;

		this->_glVersionLearned = true;

		glfwDestroyWindow(windowP);
		glfwWindowHint(GLFW_VISIBLE, GL_TRUE); //Make the next created window visible

		return true;
	}

} // namespace Private


namespace Util {


	unsigned int getMsaaSampleCount(void) {
		return CX::Private::State->getMsaaSampleCount();
	};


	GLVersion getHighestOpenGLVersion(void) {
		return CX::Private::State->getHighestOpenGLVersion();
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

/*! This function initializes CX functionality. It should probably only be called once, at program start.
\param config The intial CX configuration.
\return `true` if intialization was successful, `false` if there was an error. If there was an error, it should be logged.
*/
bool initCX(CX_InitConfiguation config) {

	if (!CX::Private::State) {
		CX::Private::State = CX::Private::globalStateFactory();
	}

	// Clear initialization state
	CX::Private::State->setCXIntialization(false);
	CX::Private::State->setInitConfig(CX_InitConfiguation());

	// Start setting up oF
	ofInit();
	ofSetEscapeQuitsApp(false);

	// Set up the clock
	config.clockPrecisionTestIterations = std::max<unsigned int>(config.clockPrecisionTestIterations, 10000);
	CX::Instances::Clock.setup(nullptr, true, config.clockPrecisionTestIterations);

	// Set up logging
	CX::Instances::Log.captureOFLogMessages(config.captureOFLogMessages);
	CX::Instances::Log.levelForAllModules(CX_Logger::Level::LOG_ALL);
	CX::Instances::Log.levelForModule(CX_Logger::Level::LOG_NOTICE, "ofShader"); // Suppress verbose shader gobbeldygook.


	// Learn openGL version. Should come before reopenWindow().
	if (!CX::Private::State->learnHighestOpenGLVersion()) {
		CX::Instances::Log.error("CX_EntryPoint") << "initCX(): Error learning the highest OpenGL version.";
	}

	// Open the window
	bool openedSucessfully = CX::Util::reopenWindow(config.windowConfig);

	if (!openedSucessfully) {
		CX::Instances::Log.error("CX_EntryPoint") << "initCX(): The window was not opened successfully.";
	} else {

		// Poll input events so that the window is at least minimally responding and doesn't get killed by the OS.
		// This must happen after the window is configured because it relies on GLFW.
		CX::Instances::Input.pollEvents(); 

		// Estimate display frame period
		if (config.framePeriodEstimationInterval > CX_Millis(0)) {
			CX::Instances::Disp.estimateFramePeriod(config.framePeriodEstimationInterval);

			CX::Instances::Disp.setFramePeriod(CX::Instances::Disp.getFramePeriod(), true);

			CX::Instances::Log.notice("CX_EntryPoint") << "initCX(): Estimated display frame rate " <<
				CX::Instances::Disp.getFrameRate() << " frames per second (" <<
				CX::Instances::Disp.getFramePeriod().millis() << " milliseconds per frame).";
		}

	}

	CX::Instances::Log.verbose() << std::endl << std::endl << "### End of startup logging data ###" << std::endl << std::endl;
	CX::Instances::Log.flush(); // Flush logs after setup so user can see if any errors happened during setup.

	// By default, suppress verbose messages to the console
	CX::Instances::Log.levelForConsole(CX_Logger::Level::LOG_NOTICE);

	// It isn't clear that this should be here, but the fbos
	// are really verbose (with notices) when allocated and it is a lot of gibberish.
	//CX::Instances::Log.levelForModule(CX_Logger::Level::LOG_WARNING, "ofFbo"); 

	CX::Private::State->setInitConfig(config);
	CX::Private::State->setCXIntialization(true);

	if (config.resetStartTime) {
		CX::Instances::Clock.resetExperimentStartTime();
	}

	return openedSucessfully;
}

bool exitCX(bool force) {

	CX::Instances::Log.flush();

	if (force) {
		glfwTerminate();
	}
	//ofExit(); // ?

	return true;
}

bool isCXInitialized(void) {
	return CX::Private::State->isCXInitialized();
}



namespace Private {

void setupGLRenderer(std::shared_ptr<ofAppBaseWindow> window) {

	std::shared_ptr<ofBaseRenderer> newRenderer = std::shared_ptr<ofBaseRenderer>(new ofGLRenderer(window.get()));

	std::dynamic_pointer_cast<ofGLRenderer>(newRenderer)->setup();
	ofSetCurrentRenderer(newRenderer, true);

	window->renderer() = newRenderer; //This seems to be unnecessary as of 0.10.1
}

void setDesiredRenderer(const CX_WindowConfiguration& config, bool setDefaultRendererIfNoneDesired, std::shared_ptr<ofAppBaseWindow> window) {

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

		std::shared_ptr<ofBaseRenderer> newRenderer = std::shared_ptr<ofBaseRenderer>(new ofGLProgrammableRenderer(window.get()));
		std::dynamic_pointer_cast<ofGLProgrammableRenderer>(newRenderer)->setup(config.desiredOpenGLVersion.major, config.desiredOpenGLVersion.minor);
		ofSetCurrentRenderer(newRenderer, true);
		window->renderer() = newRenderer; //This seems to be unnecessary as of 0.10.1

	} else {
		setupGLRenderer(window);
	}
}


bool exitCallbackHandler(ofEventArgs& args) {

	CX::Instances::Log.notice("CX_EntryPoint") << "Exit event notified.";

	ofNotifyEvent(CX::Private::getEvents().exitEvent); // Only use of CX::Private::getEvents(). Remove?

	CX::exitCX(); // maybe have extra argument inExitCallback to terminateCX?

#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 10 && OF_VERSION_PATCH == 0
	glfwTerminate(); // This is not supposed to be called from callbacks
	// but without calling this, the window hangs on 0.10.0.
	// Not needed on 0.10.1.
#endif

	return true;
}

#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR >= 10 && OF_VERSION_PATCH >= 0

void reopenWindow_0_10_0(const CX_WindowConfiguration& config) {
	
	ofWindowSettings winSet;
	

	ofGLFWWindowSettings settings;
	settings.windowMode = config.mode;
	settings.glVersionMajor = config.desiredOpenGLVersion.major;
	settings.glVersionMinor = config.desiredOpenGLVersion.minor;
	settings.numSamples = config.msaaSampleCount;
	settings.resizable = config.resizeable;
	settings.setSize(config.width, config.height);

	

	//settings.multiMonitorFullScreen = true; // bad timing
	//settings.monitor = 1;
	//settings.shareContextWith = //other window
	//settings.stereo = true; // wow
	//settings.visible = true;
	// fill more?

	std::shared_ptr<ofAppBaseWindow> curWin = ofGetMainLoop()->getCurrentWindow();
	if (!curWin) {
		ofGetMainLoop()->createWindow(settings);
		curWin = ofGetMainLoop()->getCurrentWindow();
		if (!curWin) {
			// uh oh
		}
	} else {
		curWin->close();

		curWin->setup(settings);
	}

	curWin->events().enable();

	CX::Private::setDesiredRenderer(config, true, curWin);

	ofAddListener(ofEvents().exit, &exitCallbackHandler, ofEventOrder::OF_EVENT_ORDER_AFTER_APP);

	//ofAppGLFWWindow doesn't show the window until the first call to update()
	glfwShowWindow(glfwGetCurrentContext());

	//
	CX::Private::State->setAppWindow(curWin);
}

void reopenWindow_0_10_0_old(const CX_WindowConfiguration& config) {

	bool firstCall = (CX::Private::State->getAppWindow() == nullptr);

	if (firstCall) {
		CX::Private::State->setAppWindow(std::shared_ptr<ofAppBaseWindow>(new ofAppGLFWWindow));
	} else {
		CX::Private::State->getAppWindow()->close();
	}

	std::shared_ptr<ofAppBaseWindow> baseWindow = Private::State->getAppWindow();
	//baseWindow->setup()

	std::shared_ptr<ofAppGLFWWindow> awp = std::dynamic_pointer_cast<ofAppGLFWWindow>(CX::Private::State->getAppWindow());

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
		ofGetMainLoop()->addWindow<ofAppGLFWWindow>(awp);
	}

	if (awp->getGLFWWindow() != nullptr) {

		CX::Private::setDesiredRenderer(config, true, CX::Private::State->getAppWindow());

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
bool Util::reopenWindow(CX_WindowConfiguration config) {

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

	CX::Private::State->glfwContextManager.setup(glfwGetCurrentContext(), std::this_thread::get_id());

	//Setup the display for the new window
	CX::Instances::Disp.setup();

	ofSetWindowTitle(config.windowTitle);

	return true;
}

} //namespace CX


#ifndef CX_NO_MAIN
int main(void) {
	CX::initCX(CX::CX_InitConfiguation());

	// Always runExperiment even if initialization failed so user code can see that fact.
	runExperiment();

	CX::exitCX();

	return 0;
}
#endif
