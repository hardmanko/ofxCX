#include "CX_TrialController.h"

using namespace CX;

CX_TrialController::CX_TrialController(void) :
	_functionIndex(0),
	_active(false)
{}

void CX_TrialController::appendFunction (std::function<int(void)> userFunction) {
	if (userFunction == nullptr) {
		Instances::Log.error("CX_TrialController") << "Attempt to add a user function that was nullptr at index " << _userFunctions.size();
		return;
	}

	_userFunctions.push_back(userFunction);
}

/*!
Clear the user functions and otherwise reset to default state.
*/
void CX_TrialController::reset (void) {
	_userFunctions.clear();
	_functionIndex = 0;
	_active = false;
}

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

/*!
Sets the current user function by index. If currentFunction is greater than the number
of functions stored in the trial controller, this will return false.
*/
bool CX_TrialController::setCurrentFunction (int currentFunction) {
	if ((unsigned int)currentFunction >= _userFunctions.size()) {
		Instances::Log.error("CX_TrialController") << "Attempt to set current function to out of range index. Size is " <<
			_userFunctions.size() << " and attempted index was " << currentFunction;
		return false;
	}
	_functionIndex = currentFunction;
	return true;
}