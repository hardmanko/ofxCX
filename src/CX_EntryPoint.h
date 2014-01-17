#ifndef _CX_NON_APP_ENTRY_POINT_H_
#define _CX_NON_APP_ENTRY_POINT_H_

#include "ofMain.h"

#include "CX_Utilities.h"

#include "CX_Clock.h" //CX_Clock.h includes an instance called CX::Clock
#include "CX_DeferredLogger.h" //Includes an instance called CX::Log
//#include "CX_Events.h"

//Make instances of these
#include "CX_Display.h"
#include "CX_SlidePresenter.h"
#include "CX_RandomNumberGenerator.h"
#include "CX_InputManager.h"

//Do not instantiate these, just include the header.
#include "CX_TrialController.h" 
#include "CX_TrialController_Class.h"
#include "CX_SoundObjectPlayer.h"
#include "CX_DataFrame.h"

namespace CX {

	struct CX_WindowConfiguration_t {

	};

	int setupWindow (CX_WindowConfiguration_t config);
	

	namespace Private {
		class App {
		public:
			void setup (void);
			void update (void);
			void exit (ofEventArgs &a);
		};
	}

	namespace Instances {
		extern CX_Display Display;
		extern CX_SlidePresenter SlidePresenter;
		extern CX_InputManager Input;
		extern CX_RandomNumberGenerator RNG;
	}

}

using namespace CX;
using namespace CX::Instances;


//Declarations of user functions. To be implemented in user code.
void setupExperiment (void);
void updateExperiment (void);

#endif //_CX_NON_APP_ENTRY_POINT_H_