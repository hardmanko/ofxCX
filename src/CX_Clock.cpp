#include "CX_Clock.h"

/*
#ifdef TARGET_WIN32
#include "Windows.h"
#else
#include "ofUtils.h" //For ofGetSystemTimeMicros()
#endif
*/

using namespace CX;

CX_Clock Instances::Clock; //Single instance of this class.

CX_Clock::CX_Clock (void) {
	_resetExperimentStartTime();
}

void CX_Clock::_resetExperimentStartTime(void) {
	//_pocoExperimentStart = Poco::Timestamp();
	_pocoExperimentStart = Poco::LocalDateTime();
	_experimentStart = std::chrono::high_resolution_clock().now();
}


CX_Micros CX_Clock::getExperimentStartTime(void) {
	return std::chrono::duration_cast<std::chrono::microseconds>(_experimentStart.time_since_epoch()).count();
}

/*!
This function returns the current time relative to the start of the experiment in microseconds.
The start of the experiment is defined by default as when the CX_Clock instance named Clock
(instantiated in this file) is constructed (typically the beginning of program execution). The 
experiment start time can be reset at any time by calling resetExperimentStartTime().
*/
CX_Micros CX_Clock::getTime(void) {
	std::chrono::high_resolution_clock::time_point t = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::microseconds>(t - _experimentStart).count();
}

/*!
This function returns the current system time in microseconds.

This cannot be converted to time/day in any meaningful way. Use getDateTimeString() for that.
*/
CX_Micros CX_Clock::getSystemTime(void) {
	return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

/*! Get a string representing the date/time of the start of the experiment.
\param format See getDateTimeString() for the definition of the format. */
std::string CX_Clock::getExperimentStartDateTimeString(std::string format) {
	return Poco::DateTimeFormatter::format(_pocoExperimentStart, format);
}


/*!
See http://pocoproject.org/docs/Poco.DateTimeFormatter.html#4684 for documentation of the format.
E.g. "%Y/%m/%d %H:%M:%S" gives "year/month/day 24HourClock:minute:second" with some zero-padding for most things.
The default "%Y-%b-%e %h-%M-%S %a" is "yearWithCentury-abbreviatedMonthName-nonZeroPaddedDay 12HourClock-minuteZeroPadded-secondZeroPadded am/pm".
*/
std::string CX_Clock::getDateTimeString (std::string format) {
	Poco::LocalDateTime localTime;
	return Poco::DateTimeFormatter::format(localTime, format);
}


/*
namespace CX {
//Does not actually have microsecond precision on some systems.
void sleepMicros(uint64_t micros) {
	std::this_thread::sleep_for(std::chrono::microseconds(micros));
}


long long currentTime(void) {
	//std::chrono::high_resolution_clock::time_point experimentStart = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point t = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::microseconds>(t - experimentStart).count();
}


long long micros(void) {
	return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

double clockPeriod(void) {
	return (double)std::chrono::high_resolution_clock::period().num / std::chrono::high_resolution_clock::period().den;
}


}
*/

CX_Millis::CX_Millis (int i) {
	CX_Micros fracPart = i % 1000;
	millis = (double)(i / 1000) + (double)fracPart / 1000;
}

CX_Millis::CX_Millis (CX_Micros t) {
	CX_Micros fracPart = t % 1000;
	millis = (double)(t / 1000) + (double)fracPart / 1000;
}

CX_Millis& CX_Millis::operator= (int i) {
	return this->operator=((CX_Micros)i);
}

CX_Millis& CX_Millis::operator= (double d) {
	millis = d;
	return *this;
}

CX_Millis& CX_Millis::operator= (CX_Micros t) {
	CX_Micros fracPart = t % 1000;
	millis = (double)(t / 1000) + ((double)fracPart / 1000);
	return *this;
}

CX_Millis::operator CX_Micros (void) {
	double temp = millis;
	CX_Micros intPart = (CX_Micros)floor(temp); //Get integer part
	temp -= intPart;
	temp = round(temp, -3, CX_RoundingConfiguration::ROUND_TO_NEAREST);
			
	CX_Micros fracPart = temp * 1000;
	return (intPart * 1000) + fracPart;
}

CX_Millis::operator double (void) {
	return millis;
}
