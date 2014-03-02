#ifndef _CX_ENTRY_POINT_H_
#define _CX_ENTRY_POINT_H_

/*! \namespace CX
This namespace contains all of the symbols related to CX.
*/

/*! \namespace CX::Draw
This namespace contains a variety of CX drawing functions.
*/

/*! \namespace CX::Instances
This namespace contains instances of some classes that are fundamental to the functioning of CX.
*/

/*! \namespace CX::Private
This namespace contains symbols that may be visible in user code but which should not be used by user code.
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

#include "CX_SoundObjectPlayer.h"
#include "CX_SoundObjectRecorder.h"
#include "CX_ModularSynth.h"

#include "CX_DataFrame.h"

#include "CX_Utilities.h"
#include "CX_UnitConversion.h"
#include "CX_Draw.h"

namespace CX {
	namespace Instances {
		extern CX_Display Display;
		extern CX_InputManager Input;
	}

	struct CX_WindowConfiguration_t {
		CX_WindowConfiguration_t(void) :
		width(800),
		height(600),
		mode(ofWindowMode::OF_WINDOW)
		{}

		int width;
		int height;

		ofWindowMode mode;
	};
}

using namespace CX;
using namespace CX::Instances;
using namespace CX::Util;

/*! \defgroup entryPoint Entry Point
The entry point provides access to a number of instances of classes that can be used by user code.
It also provides declarations (but not definitions) of two functions which the user should define
(\ref setupExperiment() and \ref updateExperiment()).
*/

/*! \fn runExperiment 
The user code should define a function with this name and type signature (takes no arguments and returns nothing). 
The user function will be called once setup is done for CX. When runExperiment returns, the program will exit.

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