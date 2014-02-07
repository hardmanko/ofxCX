#include "CX_TrialController.h"

using namespace CX;
using namespace CX::Util;

CX_TrialController::CX_TrialController(void) :
	_functionIndex(0),
	_active(false)
{}


/*! Adds a user function to the end of the list of functions to be called by the trial controller.
\param userFunction Typically a pointer to a function that takes no arguments and returns an int. 
Because it is a std::function, it can also be a lamda. */
void CX_TrialController::appendFunction (std::function<int(void)> userFunction) {
	if (userFunction == nullptr) {
		Instances::Log.error("CX_TrialController") << "Attempt to add a user function that was nullptr at index " << _userFunctions.size();
		return;
	}

	_userFunctions.push_back(userFunction);
}

/*! Clear the user functions and otherwise reset to default state. 
\note This function stops the trial controller (isActive() will return false). */
void CX_TrialController::reset (void) {
	_userFunctions.clear();
	_functionIndex = 0;
	_active = false;
}

/*! Updates the trial controller state. Each time this function is called, the user function at the 
current function index is called. If that function returns a nonzero value, the trial controller
will increment the current function index and that function will be called the next time update()
is called. If the current function index is incremented past the end of the list of functions, it
will wrap around to the beginning of the list.
\return The value returned by the user function that was called.
\note This function should probably be called every time updateExperiment() is called, although there are other use cases.
\note If not \ref isActive(), this function does nothing and returns 0.
*/
int CX_TrialController::update (void) {
	if (!_active) {
		return 0;
	}

	int result = 0;

	result = _userFunctions.at(_functionIndex)();

	if (result != 0) {
		if (++_functionIndex >= (int)_userFunctions.size()) {
			_functionIndex = 0;
		}
	}

	return result;
}

/*! Sets the current user function by index, which allows you to skip over functions or go back to a previous function.
\param currentFunction The new current function index. If this is out of range, an error will be logged and the function will return false.
\return False if the index was out of range, true otherwise. 
\note If this is called from within a user function that has been called from this instance of the CX_TrialController,
that function should return 0. If it does not, setCurrentFunction() will set the function index and then that index
will be incremented after the user function completes. However, if 0 is returned, the function index is not incremented. */
bool CX_TrialController::setCurrentFunction (int currentFunction) {
	if ((unsigned int)currentFunction >= _userFunctions.size()) {
		Instances::Log.error("CX_TrialController") << "Attempt to set current function to out of range index. Size is " <<
			_userFunctions.size() << " and attempted index was " << currentFunction;
		return false;
	}
	_functionIndex = currentFunction;
	return true;
}