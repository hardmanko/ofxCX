#pragma once

#include "ofMain.h" //Include all of the oF stuff.


#include "CX_Clock.h" //Includes CX::Instances::Clock
#include "CX_TimeUtilities.h"

#include "CX_Display.h" //Includes CX::Instances::Disp
#include "CX_Draw.h"
#include "CX_SlidePresenter.h"

#include "CX_InputManager.h" //Includes CX::Instances::Input
#include "CX_Logger.h" //Includes CX::Instances::Log
#include "CX_RandomNumberGenerator.h" //Includes CX::Instances::RNG

#include "CX_SoundBufferPlayer.h"
#include "CX_SoundBufferRecorder.h"
#include "CX_Synth.h"

#include "CX_DataFrame.h"
#include "CX_Algorithm.h"
#include "CX_Utilities.h"
#include "CX_UnitConversion.h"

#include "CX_Instructions.h"

#include "CX_EntryPoint.h"


/* \namespace CX
This namespace contains all of the symbols related to CX, except for \ref `runExperiment()`, which is not namespace qualified.
*/

/*! \namespace CX::Instances
This namespace contains instances of some classes that are fundamental to the functioning of CX.
*/

//This is the only place in a CX header file that the using directive is used with namespace CX or namespace CX::Instances,
//so if you don't like the namespace pollution, just comment out these lines.
#ifndef CX_NOT_USING_NAMESPACE_CX
using namespace CX;
using namespace CX::Instances;
#endif