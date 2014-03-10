#ifndef _CX_CLOCK_H_
#define _CX_CLOCK_H_

#include <string>
#include <vector>
#include <chrono>
#include <istream>
#include <ostream>

#include "Poco/DateTimeFormatter.h"

#include "CX_Utilities.h"
#include "CX_Logger.h"
#include "CX_ClockImplementations.h"

/*! \defgroup timing Timing 
This module provides methods for timestamping events in experiments.
*/



namespace CX {

	template <typename TimeUnit>
	class CX_Time {
	public:
		CX_Time(void) :
			_nanos(0)
		{}

		CX_Time(double t) {
			_nanos = (long long)_convertCount<std::nano, TimeUnit, double>(t);
		}

		CX_Time(int t) {
			_nanos = _convertCount<std::nano, TimeUnit, long long>(t);
		}

		CX_Time(long long t) {
			_nanos = _convertCount<std::nano, TimeUnit, long long>(t);
		}

		template <typename tArg>
		CX_Time(const CX_Time<tArg>& t) {
			this->_nanos = t.nanos();
		}
		/*
		template <typename tArg>
		CX_Time<TimeUnit>& operator= (const CX_Time<tArg>& t) {
		this->_nanos = t.nanos();
		return *this;
		}
		*/

		double value(void) const {
			return _convertCount<TimeUnit, std::nano, double>(_nanos);
		}

		double hours(void) const {
			return (double)_nanos / (1e9 * 60 * 60);
		}

		double minutes(void) const {
			return (double)_nanos / (1e9 * 60);
		}

		double seconds(void) const {
			return (double)_nanos / 1e9;
		}

		double millis(void) const {
			return (double)_nanos / 1e6;
		}

		double micros(void) const {
			return (double)_nanos / 1e3;
		}

		long long nanos(void) const {
			return _nanos;
		}

		template<typename RT>
		CX_Time<TimeUnit> operator+(const CX_Time<RT>& rhs) const {
			CX_Time<TimeUnit> temp(0);
			temp._nanos = this->_nanos + rhs.nanos();
			return temp;
		}

		template<typename RT>
		CX_Time<TimeUnit> operator-(const CX_Time<RT>& rhs) const {
			CX_Time<TimeUnit> temp(0);
			temp._nanos = this->_nanos - rhs.nanos();
			return temp;
		}

		template<typename RT>
		double operator/(const CX_Time<RT>& rhs) const {
			double r = (double)this->_nanos / (double)rhs.nanos();
			return r;
		}

		//The semantics of the two operator/'s are totally fucked. One divides by a scalar and gives time, 
		//the other divides by time and gives a unitless ratio. This is NON-OBVIOUS to non-mathematical people.
		CX_Time<TimeUnit> operator/(double rhs) const {
			return CX_Nanos(_nanos / rhs);
		}

		CX_Time<TimeUnit> operator*(const double rhs) const {
			double n = this->_nanos * rhs;
			return CX_Nanos(n);
		}

		CX_Time<TimeUnit>& operator*=(const double rhs) {
			this->_nanos *= rhs;
			return *this;
		}

		template<typename RT>
		CX_Time<TimeUnit>& operator+=(const CX_Time<RT>& rhs) {
			this->_nanos += rhs.nanos();
			return *this;
		}

		template<typename RT>
		CX_Time<TimeUnit>& operator-=(const CX_Time<RT>& rhs) {
			this->_nanos -= rhs.nanos();
			return *this;
		}

		template <typename RT>
		bool operator < (const CX_Time<RT>& rhs) const {
			return this->_nanos < rhs.nanos();
		}

		template <typename RT>
		bool operator <= (const CX_Time<RT>& rhs) const {
			return this->_nanos <= rhs.nanos();
		}

		template <typename RT>
		bool operator >(const CX_Time<RT>& rhs) const {
			return this->_nanos > rhs.nanos();
		}

		template <typename RT>
		bool operator >= (const CX_Time<RT>& rhs) const {
			return this->_nanos >= rhs.nanos();
		}

		template <typename RT>
		bool operator == (const CX_Time<RT>& rhs) const {
			return this->_nanos == rhs.nanos();
		}

		template <typename RT>
		bool operator != (const CX_Time<RT>& rhs) const {
			return this->_nanos != rhs.nanos();
		}

	private:
		long long _nanos;

		template<typename tOut, typename tIn, typename resultT>
		static resultT _convertCount(resultT countIn) {
			return countIn * (((double)tIn::num * tOut::den) / (tIn::den * tOut::num));
		}

		template<>
		static long long _convertCount<std::nano, std::nano, long long>(long long countIn) {
			return countIn; // *(((double)tIn::num * tOut::den) / (tIn::den * tOut::num));
		}

	};


	template <typename TimeUnit>
	std::ostream& operator<< (std::ostream& os, const CX_Time<TimeUnit>& t) {
		os << t.value(); //Assume sufficient precision to encode without losing nanos?
		return os;
	}

	template <typename TimeUnit>
	std::istream& operator>> (std::istream& is, CX_Time<TimeUnit>& t) {
		double value;
		is >> value;
		t = CX_Time<TimeUnit>(value);
		return is;
	}

	typedef CX_Time<std::ratio<3600, 1> > CX_Hours;
	typedef CX_Time<std::ratio<60, 1> > CX_Minutes;
	typedef CX_Time<std::ratio<1, 1> > CX_Seconds;
	typedef CX_Time<std::ratio<1, 1000> > CX_Millis;
	typedef CX_Time<std::ratio<1, 1000000> > CX_Micros;
	typedef CX_Time<std::ratio<1, 1000000000> > CX_Nanos;

	/*! This class is responsible for getting timestamps for anything requiring timestamps. The way to
	get timing information is the function getTime(). It returns the current time relative to the start
	of the experiment in microseconds (on most systems, see getTickPeriod() to check the actual precision).

	An instance of this class is preinstantiated for you. See CX::Instances::Clock.
	\ingroup timing
	*/
	class CX_Clock {
	public:
		CX_Clock (void);

		void setImplementation(CX::CX_BaseClock* impl);

		void precisionTest(unsigned int iterations);

		CX_Millis getTime(void);

		std::string getExperimentStartDateTimeString(std::string format = "%Y-%b-%e %h-%M-%S %a");
		static std::string getDateTimeString (std::string format = "%Y-%b-%e %h-%M-%S %a");

	private:
		Poco::LocalDateTime _pocoExperimentStart;

		CX::CX_BaseClock* _impl;
	};

	namespace Instances {
		extern CX_Clock Clock;
	}



	/*! This class can be used for profiling event loops.

	\code{.cpp}
	//Set up collection:
	CX_LapTimer lt;
	lt.setup(&Clock, 1000); //Every 1000 samples, the results of those samples will be logged.

	//In the loop:
	while (whatever) {
		//other code...
		lt.takeSample();
		//other code...
	}
	Log.flush(); //Check the results of the profiling.

	\endcode
	*/
	class CX_LapTimer {
	public:
		void setup(CX_Clock *clock, unsigned int samples);

		void reset(void);

		void takeSample(void);

		CX_Millis getAverage(void);
		CX_Millis getMinimum(void);
		CX_Millis getMaximum(void);

		std::string getStatString(void);

	private:
		CX_Clock *_clock;
		vector<CX_Millis> _timePoints;
		unsigned int _sampleIndex;
	};

}

#endif //_CX_CLOCK_H_
