/*! 
\class CX::CX_TrialController
This class is used to help with the fact that most psychology experiments
are by nature more or less linear, but that CX requires non-blocking updating
of the experiment.

The way this works is that blocks of user code, each representing one part of
a trial, are put into functions. Those functions are added to the trial controller
with appendFunction(), which puts the function at the end of the list of functions.
User functions take no arguments and return an int.

When you want to use the trial controller, call start() and it will be "armed".
When it is armed and update() is called, it will call the current function in
the list of user functions. If the user function is done with
whatever it needs to do, it should return 1. This will cause the trial controller
to move on to the next user function. If the user function is not done with its task,
it should return 0. If it returns 0, it will be called again the next time update()
is called.

*/

#ifndef _CX_TRIAL_CONTROLLER_H_
#define _CX_TRIAL_CONTROLLER_H_

#include <vector>
#include <functional>

#include "CX_Logger.h"

namespace CX {

	class CX_TrialController {
	public:
		CX_TrialController (void);

		int update (void);

		void start(void) { _active = true; };
		void stop(void) { _active = false; };
	
		void appendFunction (std::function<int(void)> userFunction);
		void reset (void);

		bool setCurrentFunction (int currentFunction);
		int currentFunction (void) { return _functionIndex; };

	private:
		int _functionIndex;

		bool _active;

		std::vector< std::function<int(void)> > _userFunctions;
	};

}

#endif //_CX_TRIAL_CONTROLLER_H_