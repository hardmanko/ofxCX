#ifndef _CX_CLOCK_H_
#define _CX_CLOCK_H_

#include <string>
#include <chrono>

#include "Poco/DateTimeFormatter.h"

#include "CX_Utilities.h"
#include "CX_Logger.h"


namespace CX {

	typedef long long CX_Micros;

	struct CX_Millis {

		CX_Millis (int i);
		CX_Millis (double d) : millis(d)  {}
		CX_Millis (CX_Micros t);

		CX_Millis& operator= (int i);
		CX_Millis& operator= (double d);
		CX_Millis& operator= (CX_Micros t);

		operator CX_Micros (void);
		operator double (void);
	private:
		double millis;
	};
	

	class CX_Clock {
	public:

		CX_Clock (void);

		CX_Micros getTime(void);
		CX_Micros getSystemTime(void);

		CX_Micros getExperimentStartTime(void);
		std::string getExperimentStartDateTimeString(std::string format = "%Y-%b-%e %h-%M-%S %a");

		static std::string getDateTimeString (std::string format = "%Y-%b-%e %h-%M-%S %a");

		double getTickPeriod(void);

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
