#pragma once

#include <vector>
#include <functional>

#include "CX_Logger.h"

namespace CX {

	template <class T>
	class CX_TrialController_Class {
	public:
		CX_TrialController_Class (void);

		bool setup (T *instance);
		int update (void);
		void reset (void);
	
		void appendFunction (int (T::*userFunction)(void));

		int currentFunction (void) { return _functionIndex; };

	private:
		T *_instance;

		int _functionIndex;

		std::vector < int (T::*)(void) > _userFunctions;
	};

	template <class T>
	CX_TrialController_Class<T>::CX_TrialController_Class (void) :
		_instance(NULL),
		_functionIndex(0)
	{
	}

	template <class T>
	bool CX_TrialController_Class<T>::setup (T *instance) {
		if (instance != NULL) {
			reset();
			_instance = instance;
			return true;
		}
		return false;
	}

	template <class T>
	void CX_TrialController_Class<T>::reset (void) {
		_instance = NULL;
		_userFunctions.clear();
		_functionIndex = 0;
	}

	template <class T>
	void CX_TrialController_Class<T>::appendFunction (int (T::*userFunction)(void)) {
		_userFunctions.push_back(userFunction);
	}

	template <class T>
	int CX_TrialController_Class<T>::update (void) {
		if (!_instance) {
			CX::Instances::Log.error("CX_TrialController_Class") << "Update called without a valid instance from which to call member functions!";
			return std::numeric_limits<int>::min();
		}

		int result = (_instance->*_userFunctions.at(_functionIndex))();

		if (result != 0) {
			if (++_functionIndex >= _userFunctions.size()) {
				_functionIndex = 0;
			}
		}

		return result;
	}

}

/*
//Simple example of how to do this stuff:

template <class T>
int callFunc (T *object, int (T::*func)(void) ) {
	return (object->*func)();
}
*/
