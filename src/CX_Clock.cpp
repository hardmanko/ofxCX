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
	_resetExperimentStartTime();
}

void CX_Clock::precisionTest(void) {
	std::vector<CX_InternalClockType::time_point> timePoints(100001);

	for (unsigned int i = 0; i < timePoints.size(); i++) {
		timePoints[i] = CX_InternalClockType::now();
	}

	std::vector<long long> durations(timePoints.size() - 1);

	uint64_t differenceSum = 0;
	long long maxDifference = 0;
	long long minDifference = std::numeric_limits<long long>::max();
	long long minNonzeroDuration = std::numeric_limits<long long>::max();

	for (unsigned int i = 0; i < durations.size(); i++) {

		long long duration = std::chrono::duration_cast<std::chrono::nanoseconds>(timePoints[i + 1] - timePoints[i]).count();
		durations[i] = duration;


		differenceSum += duration;

		if (duration > maxDifference) {
			maxDifference = duration;
		}

		if (duration < minDifference) {
			minDifference = duration;
		}

		if (duration != 0 && duration < minNonzeroDuration) {
			minNonzeroDuration = duration;
		}
	}

	cout << "Precision test results (Min, min-nonzero, mean, max): " <<
		minDifference << ", " << minNonzeroDuration << ", " << differenceSum / (timePoints.size() - 1) << ", " << maxDifference;

	//Convert the tick period to microseconds. If it is less than the measured minimum nonzero duration,
	//then it is misrepresented my the system clock.
	if (_getTheoreticalTickPeriod() * 1000000000 < minNonzeroDuration) {
		CX::Instances::Log.warning("CX_Clock") << "The system clock appears to be misrepresenting its precision.";
	}

	if (minNonzeroDuration > 1000) {
		CX::Instances::Log.warning("CX_Clock") << "The precision of the system clock used by CX_Clock is worse than "
			"microsecond precision. Actual tick period of the system clock is " << minNonzeroDuration/1000 << " microseconds";
	}
}



/*! Get the start time of the experiment in system time. The returned value can be compared with the result of getSystemTime(). */
CX_Micros CX_Clock::getExperimentStartTime(void) {
	return std::chrono::duration_cast<std::chrono::microseconds>(_experimentStart.time_since_epoch()).count();
}

/*! This function returns the current time relative to the start of the experiment in microseconds.
The start of the experiment is defined by default as when the CX_Clock instance named Clock
(instantiated in this file) is constructed (typically the beginning of program execution). */
CX_Micros CX_Clock::getTime(void) {
	CX_InternalClockType::time_point t = CX_InternalClockType::now();
	return std::chrono::duration_cast<std::chrono::microseconds>(t - _experimentStart).count();
}

/*!
This function returns the current system time in microseconds.

This cannot be converted to time/day in any meaningful way. Use getDateTimeString() for that.
\return A time value that can be compared to the result of other calls to this function and to getExperimentStartTime().
*/
CX_Micros CX_Clock::getSystemTime(void) {
	return std::chrono::duration_cast<std::chrono::microseconds>(CX_InternalClockType::now().time_since_epoch()).count();
}

/*! Get a string representing the date/time of the start of the experiment encoded according to a format.
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

/* Get the period of the underlying system clock used by CX_Clock. If the returned value 
is 1e-6 or less, the clock has microsecond precision or better. Otherwise, it has degraded 
precision. */
double CX_Clock::_getTheoreticalTickPeriod(void) {
	return (double)CX_InternalClockType::period().num / CX_InternalClockType::period().den;
}

void CX_Clock::_resetExperimentStartTime(void) {
	_pocoExperimentStart = Poco::LocalDateTime();
	_experimentStart = CX_InternalClockType::now();
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




void CX_LapTimer::setup(CX_Clock *clock, unsigned int samples) {
	_clock = clock;
	_timePoints.resize(samples);
	reset();
}

void CX_LapTimer::reset(void) {
	_sampleIndex = 0;
}

void CX_LapTimer::collectData(void) {

	_timePoints[_sampleIndex] = _clock->getSystemTime();

	if (++_sampleIndex == _timePoints.size()) {
		CX::Instances::Log.notice("CX_LapTimer") << "Data collected: " << getStatString();
		reset();
	}
}

std::string CX_LapTimer::getStatString(void) {
	uint64_t differenceSum = 0;
	CX_Micros maxDifference = 0;
	CX_Micros minDifference = std::numeric_limits<CX_Micros>::max();

	for (unsigned int i = 1; i < _timePoints.size(); i++) {
		long long difference = _timePoints[i] - _timePoints[i - 1];
		differenceSum += difference;

		if (difference > maxDifference) {
			maxDifference = difference;
		}

		if (difference < minDifference) {
			minDifference = difference;
		}
	}

	std::stringstream s;
	s << "min, mean, max: " << minDifference << ", " << differenceSum / (_timePoints.size() - 1) << ", " << maxDifference;
	return s.str();
}

CX_Micros CX_LapTimer::getAverage(void) {
	uint64_t differenceSum = 0;
	for (unsigned int i = 1; i < _timePoints.size(); i++) {
		differenceSum += _timePoints[i] - _timePoints[i - 1];
	}
	return differenceSum / (_timePoints.size() - 1);
}

CX_Micros CX_LapTimer::getMaximum(void) {
	CX_Micros maxDifference = 0;

	for (unsigned int i = 1; i < _timePoints.size(); i++) {
		CX_Micros difference = _timePoints[i] - _timePoints[i - 1];
		if (difference > maxDifference) {
			maxDifference = difference;
		}
	}

	return maxDifference;
}

CX_Micros CX_LapTimer::getMinimum(void) {
	CX_Micros minDifference = std::numeric_limits<CX_Micros>::max();
	for (unsigned int i = 1; i < _timePoints.size(); i++) {
		CX_Micros difference = _timePoints[i] - _timePoints[i - 1];
		if (difference < minDifference) {
			minDifference = difference;
		}
	}
	return minDifference;
}
