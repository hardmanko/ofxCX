#pragma once

#include <vector>

using namespace std;

namespace CX {

	class CX_TrialController_NonOO {
	public:
		CX_TrialController_NonOO (void);

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