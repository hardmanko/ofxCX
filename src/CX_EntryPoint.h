#pragma once

#include "ofWindowSettings.h"
#include "ofGLBaseTypes.h"

#include "CX_Time_t.h"
#include "CX_DisplayUtils.h"

#ifdef TARGET_LINUX
//so apparently sysmacros.h defines some macros with these names on Linux...
#ifdef major
#undef major
#endif

#ifdef minor
#undef minor
#endif
#endif

namespace CX {

	// Forward delcarations: Defined later in this file
	struct CX_WindowConfiguration;
	struct CX_InitConfiguation;

	bool initCX(CX_InitConfiguation config);
	bool isCXInitialized(void);
	bool exitCX(bool force = false);

	namespace Util {

		bool reopenWindow(CX_WindowConfiguration config);

		unsigned int getMsaaSampleCount(void);

		GLVersion getHighestOpenGLVersion(void);

		bool checkOFVersion(int versionMajor, int versionMinor, int versionPatch, bool log = true);
	}


	/*! Configures windows opened with CX::reopenWindow(). */
	struct CX_WindowConfiguration {
		CX_WindowConfiguration(void) :
			mode(ofWindowMode::OF_WINDOW),
			width(800),
			height(600),
			resizeable(false),
			msaaSampleCount(4),
			windowTitle("CX Experiment")
		{}

		ofWindowMode mode; //!< The mode of the window. One of ofWindowMode::OF_WINDOW, ofWindowMode::OF_FULLSCREEN, or ofWindowMode::OF_GAME_MODE.

		int width; //!< The width of the window, in pixels.
		int height; //!< The height of the window, in pixels.

		bool resizeable; //!< Whether or not the window can be resized by the user (i.e. by clicking and dragging the edges). Recommend `false`.

		unsigned int msaaSampleCount; //!< See CX::Util::getMsaaSampleCount(). If this value is too high, some types of drawing take a really long time.

		std::string windowTitle; //!< A title for the window that is opened.

		/*! \brief If you want to request a specific renderer, you can provide one here.
		If nothing is provided, a reasonable default is assumed. */
		std::shared_ptr<ofBaseGLRenderer> desiredRenderer;

		/*! \brief If you want to request a specific OpenGL version, you can provide this value.
		If nothing is provided, the newest OpenGL version available is used. */
		Util::GLVersion desiredOpenGLVersion;
	};

	struct CX_InitConfiguation {

		CX_InitConfiguation(void) :
			captureOFLogMessages(true),
			resetStartTime(true),
			framePeriodEstimationInterval(CX_Seconds(1)),
			clockPrecisionTestIterations(100000)
		{}

		CX_WindowConfiguration windowConfig; //!< The window configuration.

		bool captureOFLogMessages; //!< If `true`, openFrameworks log messages are captured by CX::Instances::Log. Default `true` and recommend `true`.

		bool resetStartTime; //!< If `true`, the experiment start time is reset on initialization. Default `true`.

		/*! The amount of time to spend estimating the display frame period. Passed to CX_Display::estimateFramePeriod(). */
		CX_Millis framePeriodEstimationInterval;

		/*! The number of samples of clock timing data to use to test the clock precision.
		Precision testing is very fast, so this can be in the range of 100,000 to 1,000,000 without problem.
		Forced to be at least 10,000.
		Passed to CX_Clock::chooseBestClockImplementation(). */
		unsigned int clockPrecisionTestIterations;
	};

	/*! The Private namespace contains symbols that may be visible in user code 
	but which should not be used by user code (and expect good results). */
	namespace Private {

		class CX_GlobalState {

			friend std::shared_ptr<CX_GlobalState> globalStateFactory(void);
			CX_GlobalState(void) :
				_firstInitialization(true),
				_cxIsInitialized(false),
				_glVersionLearned(false)
			{}

			bool _firstInitialization;
			bool _cxIsInitialized;
			CX_InitConfiguation _initConfig;

			bool _glVersionLearned;
			Util::GLVersion _maxGLVersion;

			std::shared_ptr<ofAppBaseWindow> _appWindow;

		public:

			void setCXIntialization(bool cxInitialized);
			bool isCXInitialized(void) const;
			bool hasCXBeenInitialized(void) const;

			void setInitConfig(const CX_InitConfiguation& cfg);
			const CX_InitConfiguation& getInitConfig(void) const;
			//bool cxIsInitialized(void) const;
			unsigned int getMsaaSampleCount(void) const;

			bool learnHighestOpenGLVersion(void);
			//void setMaxGLVersion(const CX_GLVersion& ver);
			const Util::GLVersion& getHighestOpenGLVersion(void) const;

			void setAppWindow(std::shared_ptr<ofAppBaseWindow> wind);
			std::shared_ptr<ofAppBaseWindow> getAppWindow(void) const;

			// Doesn't need set/get
			//std::unique_ptr<Util::GlfwContextManager> glfwContextManager;
			Util::GlfwContextManager glfwContextManager;

		};

		std::shared_ptr<CX_GlobalState> globalStateFactory(void);

		extern std::shared_ptr<CX_GlobalState> State;

	} // namespace Private

} // Namespace CX

/*! \defgroup entryPoint Entry Point
The entry point provides access to a few instances of classes that can be used by user code.
It also provides declarations (but not definitions) of a function which the user should define
(see \ref runExperiment()).
*/

/*!
\def CX_NO_MAIN
For advanced users who want the customize how CX starts up.

If this preprocessor macro is defined, CX will not produce a `main` function, leaving it up to the user to produce such a function.
In addition, if `CX_NO_MAIN` is defined, `runExperiment()` will not be declared. This means that you will not need to define
a `runExperiment()` function.

A `main` function can be as simple as:

\code{cpp}
void main (void) {
CX::initCX(CX_InitConfiguation());

// Your experiment goes here...

CX::terminateCX();
}
\endcode

\ingroup entryPoint
*/
#ifdef DOXYGEN
#define CX_NO_MAIN
#endif

/*! \fn runExperiment
The user code should define a function with this name and type signature (takes no arguments and returns nothing).
This function will be called once setup is done for CX. When `runExperiment` returns, the program will exit.

\code{.cpp}
void runExperiment (void) {
	//Do your experiment.

	return; //Return when done to exit the program. You don't have to explicity return; you can just fall off the end of the function.
		//You can alternately call std::exit() at any point.
}
\endcode

\ingroup entryPoint
*/
#ifndef CX_NO_MAIN
void runExperiment(void);
#endif
