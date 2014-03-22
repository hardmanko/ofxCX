#ifndef _CX_ENTRY_POINT_H_
#define _CX_ENTRY_POINT_H_

/*! \namespace CX
This namespace contains all of the symbols related to CX.
*/

/*! \namespace CX::Instances
This namespace contains instances of some classes that are fundamental to the functioning of CX.
*/

#include "ofMain.h" //Include all of the oF stuff.

#include "CX_Clock.h" //CX_Clock.h includes an instance called CX::Instances::Clock
#include "CX_Logger.h" //Includes an instance called CX::Instances::Log
#include "CX_RandomNumberGenerator.h" //Includes an instance called CX::Instances::RNG

//Make instances of these
#include "CX_Display.h"
#include "CX_SlidePresenter.h"
#include "CX_InputManager.h"

//Do not instantiate these, just include the header.
#include "CX_TrialController.h" 
#include "CX_TrialController_Class.h"

#include "CX_SoundBufferPlayer.h"
#include "CX_SoundBufferRecorder.h"
#include "CX_ModularSynth.h"

#include "CX_DataFrame.h"

#include "CX_Algorithm.h"
#include "CX_Utilities.h"
#include "CX_UnitConversion.h"
#include "CX_Draw.h"

#include "CX_Private.h"

namespace CX {
	namespace Instances {
		extern CX_Display Display;
		extern CX_InputManager Input;
	}

	struct CX_WindowConfiguration_t {
		CX_WindowConfiguration_t(void) :
			mode(ofWindowMode::OF_WINDOW),
			width(800),
			height(600),
			multisampleSampleCount(4)
		{}

		ofWindowMode mode;

		int width;
		int height;

		unsigned int multisampleSampleCount;

		ofPtr<ofBaseGLRenderer> desiredRenderer;
		Private::CX_GLVersion desiredOpenGLVersion;
	};

	void relaunchWindow(CX_WindowConfiguration_t config);

}

using namespace CX;
using namespace CX::Instances;
using namespace CX::Util;

/*! \defgroup entryPoint Entry Point
The entry point provides access to a few instances of classes that can be used by user code.
It also provides declarations (but not definitions) of a function which the user should define
(see \ref runExperiment()).
*/

/*! \fn runExperiment 
The user code should define a function with this name and type signature (takes no arguments and returns nothing). 
This function will be called once setup is done for CX. When runExperiment returns, the program will exit.

\code{.cpp}
void runExperiment (void) {
	//Do your experiment.

	return; //Return when done to exit the program. You don't have to explicity return; you can just fall off the end of the function. 
		//You can alternately call std::exit() or ofExit() at any point.
}
\endcode

\ingroup entryPoint
*/
void runExperiment (void);

#endif //_CX_ENTRY_POINT_H_