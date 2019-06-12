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

	// defined later
	struct CX_GLVersion; 
	struct CX_WindowConfiguration;
	struct CX_InitConfiguation;

	bool initializeCX(CX_InitConfiguation config);
	bool reopenWindow(CX_WindowConfiguration config);

	namespace Util {

		unsigned int getMsaaSampleCount(void);

		void learnHighestOpenGLVersion(void);
		CX_GLVersion getHighestOpenGLVersion(void);

		bool checkOFVersion(int versionMajor, int versionMinor, int versionPatch, bool log = true);
	}


	/*! Stores openGL version numbers and has a few helper functions. */
	struct CX_GLVersion {
		CX_GLVersion(void) :
			major(0),
			minor(0),
			release(0)
		{}

		CX_GLVersion(int maj, int min, int rel) :
			major(maj),
			minor(min),
			release(rel)
		{}

		int major;
		int minor;
		int release;

		int compare(int maj, int min, int rel) const;
		int compare(const CX_GLVersion& that) const;

		bool supportsGLFenceSync(void) const;
		//bool supportsGLProgrammableRenderer(void) const;

		CX_GLVersion getCorrespondingGLSLVersion(void) const;

	};

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

		bool resizeable; //!< Whether or not the window can be resized by the user (i.e. by clicking and dragging the edges).

		unsigned int msaaSampleCount; //!< See CX::Util::getMsaaSampleCount(). If this value is too high, some types of drawing take a really long time.

		std::string windowTitle; //!< A title for the window that is opened.

		/*! \brief If you want to request a specific renderer, you can provide one here.
		If nothing is provided, a reasonable default is assumed. */
		std::shared_ptr<ofBaseGLRenderer> desiredRenderer;

		/*! \brief If you want to request a specific OpenGL version, you can provide this value.
		If nothing is provided, the newest OpenGL version available is used. */
		CX_GLVersion desiredOpenGLVersion;
	};

	struct CX_InitConfiguation {

		CX_InitConfiguation(void) :
			captureOFLogMessages(true),
			framePeriodEstimationInterval(CX_Seconds(1)),
			clockPrecisionTestIterations(100000)
		{}

		CX_WindowConfiguration windowConfig; //!< The window configuration.

		bool captureOFLogMessages; //!< If `true`, openFrameworks log messages are captured by CX::Instances::Log (recommend `true`).

		/*! The amount of time to spend estimating the frame period. */
		CX_Millis framePeriodEstimationInterval;

		/*! The number of samples of clock timing data to use to test the clock precision.
		Precision testing is very fast, so this can be in the range of 100,000 without problem.
		Forced to be at least 10,000.
		Passed to CX_Clock::chooseBestClockImplementation(). */
		unsigned int clockPrecisionTestIterations;
	};

	namespace Private {

		struct CX_State {

			CX_GLVersion maxGLVersion;

			CX_InitConfiguation initConfig;

			std::shared_ptr<ofAppBaseWindow> appWindow;

			Util::CX_GlfwContextManager glfwContextManager;
		};

		extern CX_State State;
	}

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
CX::initializeCX(CX_InitConfiguation());

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
