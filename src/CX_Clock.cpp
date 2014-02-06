#include "CX_Clock.h"

using namespace CX;

/*! An instance of CX::CX_Clock that is hooked into the CX backend. Anything in CX
that requires timing information uses this instance. You should use this instance in
your code and not make your own instance of CX_Clock. You should never need another instance. 
You should never use another instance, as the experiment start times will not agree between 
instances.
\ingroup timing */
CX_Clock CX::Instances::Clock;

CX_Clock::CX_Clock (void) {
	if (this->getTickPeriod() > 1e-6) {
		Instances::Log.warning("CX_Clock") << "The precision of the system clock used by CX_Clock is worse than "
			"microsecond precision. Actual tick period of the system clock is " << this->getTickPeriod();
	}
	_resetExperimentStartTime();
}

void CX_Clock::_resetExperimentStartTime(void) {
	_pocoExperimentStart = Poco::LocalDateTime();
	_experimentStart = std::chrono::high_resolution_clock().now();
}

/*! Get the start time of the experiment in system time. The returned value can be compared with the result of getSystemTime(). */
CX_Micros CX_Clock::getExperimentStartTime(void) {
	return std::chrono::duration_cast<std::chrono::microseconds>(_experimentStart.time_since_epoch()).count();
}

/*! This function returns the current time relative to the start of the experiment in microseconds.
The start of the experiment is defined by default as when the CX_Clock instance named Clock
(instantiated in this file) is constructed (typically the beginning of program execution). */
CX_Micros CX_Clock::getTime(void) {
	std::chrono::high_resolution_clock::time_point t = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::microseconds>(t - _experimentStart).count();
}

/*!
This function returns the current system time in microseconds.

This cannot be converted to time/day in any meaningful way. Use getDateTimeString() for that.
This value can only be compared to the result of other calls to this function and to getExperimentStartTime().
*/
CX_Micros CX_Clock::getSystemTime(void) {
	return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

/*! Get a string representing the date/time of the start of the experiment encoded according to some format.
\param format See getDateTimeString() for the definition of the format. */
std::string CX_Clock::getExperimentStartDateTimeString(std::string format) {
	return Poco::DateTimeFormatter::format(_pocoExperimentStart, format);
}


/*! This function returns a string containing the local time encoded according to some format.
\param format See http://pocoproject.org/docs/Poco.DateTimeFormatter.html#4684 for documentation of the format.
E.g. "%Y/%m/%d %H:%M:%S" gives "year/month/day 24HourClock:minute:second" with some zero-padding for most things.
The default "%Y-%b-%e %h-%M-%S %a" is "yearWithCentury-abbreviatedMonthName-nonZeroPaddedDay 12HourClock-minuteZeroPadded-secondZeroPadded am/pm".
*/
std::string CX_Clock::getDateTimeString (std::string format) {
	Poco::LocalDateTime localTime;
	return Poco::DateTimeFormatter::format(localTime, format);
}

/*! Get the period of the underlying system clock used by CX_Clock. If the returned value 
is 1e-6 or less, the clock has microsecond precision or better. Otherwise, it has degraded 
precision. */
double CX_Clock::getTickPeriod(void) {
	return (double)std::chrono::high_resolution_clock::period().num / std::chrono::high_resolution_clock::period().den;
}

/*
//Does not actually have microsecond precision on some systems.
void sleepMicros(uint64_t micros) {
	std::this_thread::sleep_for(std::chrono::microseconds(micros));
}


double clockPeriod(void) {
	return (double)std::chrono::high_resolution_clock::period().num / std::chrono::high_resolution_clock::period().den;
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
	temp = CX::Util::round(temp, -3, CX::Util::CX_RoundingConfiguration::ROUND_TO_NEAREST);
			
	CX_Micros fracPart = temp * 1000;
	return (intPart * 1000) + fracPart;
}

CX_Millis::operator double (void) {
	return millis;
}
