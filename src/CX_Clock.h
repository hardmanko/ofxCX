#ifndef _CX_CLOCK_H_
#define _CX_CLOCK_H_

#include "ofConstants.h"
#include "ofLog.h"

#include <stdint.h>
#include <string>

#include "Poco/DateTimeFormatter.h"

namespace CX {

	class CX_Clock {
	public:

		CX_Clock (void);

		uint64_t getTime (void);
		uint64_t getSystemTime (void);

		uint64_t getExperimentStartTime (void) { return _experimentStartTime; };

		static std::string getDateTimeString (std::string format = "%Y/%b/%e %h:%M:%S %a");

	private:
		void _resetExperimentStartTime (void);

#ifdef TARGET_WIN32
		uint64_t _performanceCounterFrequency;
#endif

		uint64_t _experimentStartTime;
	};

	namespace Instances {
		extern CX_Clock Clock; //Single instance of this class. You should never need another instance. 
			//You should never use another instance, as the experiment start times will not agree.
	}
}

#endif //_CX_CLOCK_H_
