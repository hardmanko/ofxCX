#ifndef _CX_TRIAL_CONTROLLER_H_
#define _CX_TRIAL_CONTROLLER_H_

#include <vector>

using namespace std;

namespace CX {

	template <class T>
	class CX_TrialController {
	public:
		CX_TrialController (void);

		bool setup (T *instance);
		int update (void);
		void reset (void);
	
		void appendFunction (int (T::*userFunction)(void));

		int currentFunction (void) { return _functionIndex; };

	private:
		T *_instance;

		int _functionIndex;

		vector < int (T::*)(void) > _userFunctions;

	};

	template <class T>
	CX_TrialController<T>::CX_TrialController (void) :
		_instance(NULL),
		_functionIndex(0)
	{
	}

	template <class T>
	bool CX_TrialController<T>::setup (T *instance) {
		if (instance != NULL) {
			reset();
			_instance = instance;
			return true;
		}
		return false;
	}

	template <class T>
	void CX_TrialController<T>::reset (void) {
		_instance = NULL;
		_userFunctions.clear();
		_functionIndex = 0;
	}

	template <class T>
	void CX_TrialController<T>::appendFunction (int (T::*userFunction)(void)) {
		_userFunctions.push_back(userFunction);
	}

	template <class T>
	int CX_TrialController<T>::update (void) {
		if (!_instance) {
			ofLogError("CX_TrialController") << "Update called without a valid instance from which to call member functions!";
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

#endif //_CX_TRIAL_CONTROLLER_H_