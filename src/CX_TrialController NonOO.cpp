#include "CX_TrialController NonOO.h"

using namespace CX;

CX_TrialController_NonOO::CX_TrialController_NonOO (void) :
	_functionIndex(0)
{
}

void CX_TrialController_NonOO::appendFunction (int (*userFunction)(void)) {
	_userFunctions.push_back(userFunction);
}

void CX_TrialController_NonOO::reset (void) {
	_userFunctions.clear();
	_functionIndex = 0;
}

int CX_TrialController_NonOO::update (void) {

	int result = _userFunctions.at(_functionIndex)();

	if (result != 0) {
		if (++_functionIndex >= (int)_userFunctions.size()) {
			_functionIndex = 0;
		}
	}

	return result;
}

bool CX_TrialController_NonOO::setCurrentFunction (int currentFunction) {
	if ((unsigned int)currentFunction >= _userFunctions.size()) {
		return false;
	}
	_functionIndex = currentFunction;
	return true;
}