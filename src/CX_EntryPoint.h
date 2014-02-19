#ifndef _CX_ENTRY_POINT_H_
#define _CX_ENTRY_POINT_H_

/*! \mainpage CX Manual

ofxCX (hereafter referred to as CX) is a "total conversion mod" for openFrameworks (often abbreviated oF) that 
is designed to be used used for creating psychology experiments.

The most well-organized way to access the documentation is go to the \ref modules page. The best way to get
an overview of how CX works is to look at the \ref examplesAndTutorials.

To find out more about presenting visual stimuli, go to the \ref video page.

To find out about auditory stimuli, go to the \ref sound page.

To learn about how CX logs errors, see the \ref errorLogging page.

To learn about how to store and output experiment data, see the \ref dataManagement page.

To learn about random number generation, see the \ref randomNumberGeneration page.


\section install Installation
In order to use CX, you must have openFrameworks installed. See http://openframeworks.cc/download/ to download openFrameworks.
Currently only version 0.8.0 of openFrameworks is supported by CX.

Once you have installed openFrameworks, you can install CX by putting the contents of this repository into a subdirectory 
under openFrameworksInstallDirectory/addons (typically openFrameworksInstallDirectory/addons/ofxCX), where openFrameworksInstallDirectory
is where you put openFramworks when you installed it.

To use CX in a project, use the openFrameworks project generator and select ofxCX as an addon to use. The project generator can
be found in openFrameworksInstallDirectory/projectGenerator.

In order to use the examples, do the following:
1. Use the oF project generator (in openFrameworksInstallDirectory/projectGenerator) to create a new project that uses the ofxCX addon.
2. Go to the newly-created project directory (that you chose when creating the project in step 1) and go into the src subdirectory.
3. Delete all of the files in the src directory (main.cpp, testApp.h, and testApp.cpp)
4. Copy the example .cpp file into this directory
4a. If the example has a data folder, copy the contents of that folder into yourProjectDirectory/bin/data. bin/data folders probably won't 
exist at this point. You can create it.
5. This step depends on your compiler, but you'll need to tell it to use the example source file (.cpp) when it compiles the project (and possibly to specifically not use the files you deleted from the src directory in step 3).
6. Compile and run the project

\section examplesAndTutorials Examples and Tutorials
There are several examples that serve as tutorials for CX. Some of the examples are on a specific topic and others 
are sample experiments that integrate together different features of CX. The example files can be found in the CX
directory (see \ref install) in subfolders with names beginning with "example-".

Tutorials:
-----------------------
+ soundObject - Tutorial covering a number of things that you can do with CX_SoundObjects, including loading sound 
files, combining sounds, and playing them.
+ dataFrame - Tutorial covering use of CX_DataFrame, which is a container for storing data that is collected in an experiment.
+ logging - Tutoral explaining how the error logging system of CX works and how you can use it in your experiments.

Experiments:
------------------------
- basicChangeDetection - A very straightforward change-detection task demonstrating some of the features of CX 
like presentation of time-locked stimuli, keyboard response collection, and use of the CX_RandomNumberGenerator.
+ advancedChangeDetection - This example expands on basicChangeDetection, including CX_DataFrame and CX_TrialController 
in order to simplify the experiment.
+ nBack - Demonstrates advanced use of CX_SlidePresenter in the implementation of an N-Back task.

Misc.:
-----------------------
+ helloWorld - A very basic getting started program.
+ animation - A simple example of the most simple way to draw moving things in CX. Also includes some mouse stuff: 
cursor movement, clicks, and scroll wheel activity.
+ renderingTest - Includes several examples of how to draw stuff using ofPath (arbitrary lines), ofTexture (a kind 
of pixel buffer), ofImage (for opening image files: .png, .jpg, etc.), a variety of basic oF drawing functions 
(ofCircle, ofRect, ofTriangle, etc.), and a number of CX drawing functions from the CX::Draw namespace.

\page modules Modules

\page blocking Blocking Code
Blocking code is code that either takes a long time to complete or that waits until some event occurs before
allowing code execution to continue. An example of blocking code that waits is

	while (Input.Keyboard.availbleEvents() == 0) {};

This code waits until the keyboard has been used in some way. No code past it can be executed until the keyboard
is used, which could take a long time. Any code that blocks while waiting for a human to do something is blocking.

An example of blocking code that takes a long time (or at least could take a long time) is

	vector<double> d = CX::Util::sequence<double>(0, 1000000, .033);

which requires the allocation of about 300 MB of RAM. This code doesn't wait for anything to happen, it
just takes a long time to execute.

Blocking code is bad because it prevents some parts of CX from working in some situations. It is not a cardinal
sin and there are times when using blocking code is acceptable. However, blocking code should
not be used when trying to present stimuli or when responses are being made. There is of course an exception to
the responses rule, which is when your blocking code is explicitly polling for user input, e.g.:

	while(!Input.pollEvents());

*/

/*! \namespace CX::Private
This namespace contains symbols that may be visible in user code but which should not be used by user code.
*/

/*! \namespace CX
This namespace contains all of the symbols related to CX.
*/

/*! \namespace CX::Draw
This namespace contains a variety of CX drawing functions.
*/

/*! \namespace CX::Instances
This namespace contains instances of some classes that are fundamental to the functioning of CX. 
CX::Instances::Clock is used throughout CX for timing purposes.
CX::Instances::Log is used throughout CX for message logging.
CX::Instances::RNG is an instance of a CX_RandomNumberGenerator.
CX::Instances::Input manages the three primary input devices (Mouse, Keyboard, and Joystick).
CX::Instances::Display is the primary display used for presenting visual stimuli.
CX::Instances::SlidePresenter is a very useful abstraction that is used for the presentation of visual stimuli.
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
#include "CX_SoundObjectPlayer.h" //All of the sound stuff comes with this header
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
		mode(ofWindowMode::OF_WINDOW),
		useVideoHardwareCompatibilityMode(false)
		{}

		int width;
		int height;

		ofWindowMode mode;

		bool useVideoHardwareCompatibilityMode;
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
The user code should define a function with this name and type signature. The user function will be called
once setup is done for CX. When runExperiment returns, the program will exit.

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