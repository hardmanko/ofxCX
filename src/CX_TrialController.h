#ifndef _CX_TRIAL_CONTROLLER_H_
#define _CX_TRIAL_CONTROLLER_H_

#include <vector>

using namespace std;

namespace CX {

	class CX_TrialController {
	public:
		CX_TrialController (void);

		int update (void);
	
		void appendFunction (int (*userFunction) (void));
		void reset (void);

		bool setCurrentFunction (int currentFunction);
		int currentFunction (void) { return _functionIndex; };

	private:
		int _functionIndex;

		vector < int (*)(void) > _userFunctions;

	};

}

#endif //_CX_TRIAL_CONTROLLER_H_