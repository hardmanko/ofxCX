#pragma once

#include "CX.h"

namespace CX {

	/*! This structure is used to configure windows opened with CX::reopenWindow(). */
	struct CX_WindowConfiguration_t {
		CX_WindowConfiguration_t(void) :
			mode(ofWindowMode::OF_WINDOW),
			width(800),
			height(600),
			msaaSampleCount(4),
			windowTitle("CX Experiment"),
			preOpeningUserFunction(nullptr)
		{}

		ofWindowMode mode; //!< The mode of the window. One of ofWindowMode::OF_WINDOW, ofWindowMode::OF_FULLSCREEN, or ofWindowMode::OF_GAME_MODE.

		int width; //!< The width of the window, in pixels.
		int height; //!< The height of the window, in pixels.

		unsigned int msaaSampleCount; //!< See CX::Util::getMsaaSampleCount(). If this value is too high, some types of drawing take a really long time.

		/*! \brief If you want to request a specific renderer, you can provide one here.
		If nothing is provided, a reasonable default is assumed. */
		ofPtr<ofBaseGLRenderer> desiredRenderer;

		/*! \brief If you want to request a specific OpenGL version, you can provide this value.
		If nothing is provided, the newest OpenGL version available is used. */
		Private::CX_GLVersion desiredOpenGLVersion;

		/*! A title for the window that is opened. */
		std::string windowTitle;

		/*! A user-supplied function that will be called just before the GLFW window is opened. This allows you to
		set window hints just before the window is opened. This only works if you are using oF version 0.8.4. */
		std::function<void(void)> preOpeningUserFunction;
	};

	bool reopenWindow(CX_WindowConfiguration_t config);

}

/*! \defgroup entryPoint Entry Point
The entry point provides access to a few instances of classes that can be used by user code.
It also provides declarations (but not definitions) of a function which the user should define
(see \ref runExperiment()).
*/

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
void runExperiment (void);
