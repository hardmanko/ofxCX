#ifndef _CX_CLOCK_H_
#define _CX_CLOCK_H_

//#include "ofConstants.h"
#include "ofLog.h"

//#include <stdint.h>
#include <string>
#include <chrono>

#include "Poco/DateTimeFormatter.h"

//#include "CX_DeferredLogger.h"

namespace CX {

	typedef long long CX_Micros_t;

	class CX_Clock {
	public:

		CX_Clock (void);

		CX_Micros_t getTime(void);
		CX_Micros_t getSystemTime(void);

		CX_Micros_t getExperimentStartTime(void);
		std::string getExperimentStartDateTimeString(std::string format = "%Y-%b-%e %h-%M-%S %a");

		static std::string getDateTimeString (std::string format = "%Y-%b-%e %h-%M-%S %a");

	private:
		void _resetExperimentStartTime (void);

		std::chrono::high_resolution_clock::time_point _experimentStart;

		Poco::LocalDateTime _pocoExperimentStart;
		
	};

	namespace Instances {
		extern CX_Clock Clock; //Single instance of this class. You should never need another instance. 
			//You should never use another instance, as the experiment start times will not agree.
	}
}

#endif //_CX_CLOCK_H_
