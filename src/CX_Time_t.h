#pragma once

#include <chrono>

//#include "CX_Utilities.h"

namespace CX {

	template <typename T> class CX_Time_t;

	typedef CX_Time_t<std::ratio<3600, 1> > CX_Hours;
	typedef CX_Time_t<std::ratio<60, 1> > CX_Minutes;
	typedef CX_Time_t<std::ratio<1, 1> > CX_Seconds;
	typedef CX_Time_t<std::ratio<1, 1000> > CX_Millis;
	typedef CX_Time_t<std::ratio<1, 1000000> > CX_Micros;
	typedef CX_Time_t<std::ratio<1, 1000000000> > CX_Nanos;

	namespace Private {
		template<typename tOut, typename tIn, typename resultT>
		inline resultT convertTimeCount(resultT countIn) {
			return countIn * (((double)tIn::num * tOut::den) / ((double)tIn::den * tOut::num));
		}

		template<>
		inline long long convertTimeCount<std::nano, std::nano, long long>(long long countIn) {
			return countIn;
		}

		template<>
		inline long long convertTimeCount<std::micro, std::micro, long long>(long long countIn) {
			return countIn;
		}

		template<>
		inline long long convertTimeCount<std::milli, std::milli, long long>(long long countIn) {
			return countIn;
		}
	}

	/*! This class provides a convenient way to deal with time in various units. The upside of this
	system is that although all functions in CX that take time can take time values in a variety of
	units. For example, CX_Clock::wait() takes CX_Millis as the time type so if you were to do
	\code{.cpp}
	Clock.wait(20);
	\endcode
	it would attempt to wait for 20 milliseconds. However, you could do
	\code{.cpp}
	Clock.wait(CX_Seconds(.5));
	\endcode
	to wait for half of a second, if units of seconds are easier to think in for the given situation.
	
	CX_Time_t has at most nanosecond accuracy. The contents of any of the templated 
	versions of CX_Time_t are all stored in nanoseconds, so conversion between time types is lossless.

	See this example for a varity of things you can do with this class.
	\code{.cpp}
	CX_Millis mil = 100;
	CX_Micros mic = mil; //mic now contains 100000 microseconds == 100 milliseconds.
	//Really, they both contain 100,000,000 nanoseconds.

	//You can add times together.
	CX_Seconds sec = CX_Minutes(.1) + CX_Millis(100); //sec contains 6.1 seconds.

	//You can take the ratio of times.
	double secondsPerMinute = CX_Seconds(1)/CX_Minutes(1);

	//You can compare times using the standard comparison operators (==, !=, <, >, <=, >=).
	if (CX_Minutes(60) == CX_Hours(1)) {
	cout << "There are 60 minutes in an hour." << endl;
	}

	if (CX_Millis(12.3456) == CX_Micros(12345.6)) {
	cout << "Time can be represented as a floating point value with sub-time-unit precision." << endl;
	}

	//If you want to be explicit about what time unit you want out, you can use the seconds(), millis(), etc., functions:
	sec = CX_Seconds(6);
	cout << "In " << sec.seconds() << " seconds there are " << sec.millis() << " milliseconds and " << sec.minutes() << " minutes." << endl;

	//You can alternately do a typecast if you're about to print the result:
	cout << "In " << sec << " seconds there are " << (CX_Millis)sec << " milliseconds and " << (CX_Minutes)sec << " minutes." << endl;

	//The difference between the above examples is the resulting type.
	//double minutes = (CX_Minutes)sec; //This does not work: A CX_Minutes cannot be assigned to a double
	double minutes = sec.minutes(); //minutes() returns a double.

	//You can construct a time with the result of the construction of a time object with a different time unit.
	CX_Minutes min = CX_Hours(.05); //3 minutes

	//You can get the whole number amounts of different time units.
	CX_Seconds longTime = CX_Hours(2) + CX_Minutes(16) + CX_Seconds(40) + CX_Millis(123) + CX_Micros(456) + CX_Nanos(1);
	CX_Seconds::PartitionedTime parts = longTime.getPartitionedTime();

	\endcode

	\ingroup timing
	*/
	template <typename TimeUnit>
	class CX_Time_t {
	public:

		/*! This struct contains the result of CX_Time_t::getPartitionedTime(). */
		struct PartitionedTime {
			int hours;
			int minutes;
			int seconds;
			int milliseconds;
			int microseconds;
			int nanoseconds;
		};

		/*! Partitions a CX_Time_t into component parts containing the number of whole time units
		that are stored in the CX_Time_t. This is different from seconds(), millis(), etc., because
		those functions return the fractional part (e.g. 5.340 seconds) whereas this returns only
		whole numbers (e.g. 5 seconds and 340 milliseconds).
		*/
		PartitionedTime getPartitionedTime(void) const {
			CX_Time_t<TimeUnit> t = *this;
			PartitionedTime rval;
			rval.hours = floor(t.hours());
			t -= CX_Hours(rval.hours);

			rval.minutes = floor(t.minutes());
			t -= CX_Minutes(rval.minutes);

			rval.seconds = floor(t.seconds());
			t -= CX_Seconds(rval.seconds);

			rval.milliseconds = floor(t.millis());
			t -= CX_Millis(rval.milliseconds);

			rval.microseconds = floor(t.micros());
			t -= CX_Micros(rval.microseconds);

			rval.nanoseconds = t.nanos();
			return rval;
		}

		CX_Time_t(void) :
			_nanos(0)
		{}

		CX_Time_t(double t) {
			_nanos = (long long)Private::convertTimeCount<std::nano, TimeUnit, double>(t);
		}

		CX_Time_t(int t) {
			_nanos = Private::convertTimeCount<std::nano, TimeUnit, long long>(t);
		}

		CX_Time_t(long long t) {
			_nanos = Private::convertTimeCount<std::nano, TimeUnit, long long>(t);
		}

		template <typename tArg>
		CX_Time_t(const CX_Time_t<tArg>& t) {
			this->_nanos = t.nanos();
		}

		/*! Get the numerical value of the time in units of the time type. For example, if
		you are using an instance of CX_Seconds, this will return the time value in seconds,
		including fractional seconds.
		*/
		double value(void) const {
			return Private::convertTimeCount<TimeUnit, std::nano, double>(_nanos);
		}

		/*! \brief Get the time stored by this CX_Time_t in hours, including fractional hours. */
		double hours(void) const {
			return (double)_nanos / (1e9 * 60 * 60);
		}

		/*! \brief Get the time stored by this CX_Time_t in minutes, including fractional minutes. */
		double minutes(void) const {
			return (double)_nanos / (1e9 * 60);
		}

		/*! \brief Get the time stored by this CX_Time_t in seconds, including fractional seconds. */
		double seconds(void) const {
			return (double)_nanos / 1e9;
		}

		/*! \brief Get the time stored by this CX_Time_t in milliseconds, including fractional milliseconds. */
		double millis(void) const {
			return (double)_nanos / 1e6;
		}

		/*! \brief Get the time stored by this CX_Time_t in microseconds, including fractional microseconds. */
		double micros(void) const {
			return (double)_nanos / 1e3;
		}

		/*! \brief Get the time stored by this CX_Time_t in nanoseconds. */
		long long nanos(void) const {
			return _nanos;
		}

		/*! \brief Adds together two times. */
		template<typename RT>
		CX_Time_t<TimeUnit> operator+(const CX_Time_t<RT>& rhs) const {
			CX_Time_t<TimeUnit> temp(0);
			temp._nanos = this->_nanos + rhs.nanos();
			return temp;
		}

		/*! \brief Subtracts two times. */
		template<typename RT>
		CX_Time_t<TimeUnit> operator-(const CX_Time_t<RT>& rhs) const {
			CX_Time_t<TimeUnit> temp(0);
			temp._nanos = this->_nanos - rhs.nanos();
			return temp;
		}

		/*! \brief Divides a CX_Time_t by another CX_Time_t, resulting in a unitless ratio. */
		template<typename RT>
		double operator/(const CX_Time_t<RT>& rhs) const {
			double r = (double)this->_nanos / (double)rhs.nanos();
			return r;
		}

		/*! \brief Divides a CX_Time_t by a unitless value, resulting in a CX_Time_t of the same type. */
		CX_Time_t<TimeUnit> operator/(double rhs) const {
			return CX_Nanos(_nanos / rhs);
		}

		/*! \brief Multiplies a CX_Time_t by a unitless value, resulting in a CX_Time_t of the same type.
		You cannot multiply a time by another time because that would result in units of time squared. */
		CX_Time_t<TimeUnit> operator*(double rhs) const {
			double n = this->_nanos * rhs;
			return CX_Nanos(n);
		}

		/*! \brief Multiplies a CX_Time_t by a unitless value, storing the result in the CX_Time_t.
		You cannot multiply a time by another time because that would result in units of time squared. */
		CX_Time_t<TimeUnit>& operator*=(double rhs) {
			this->_nanos *= rhs;
			return *this;
		}

		template<typename RT>
		CX_Time_t<TimeUnit>& operator+=(const CX_Time_t<RT>& rhs) {
			this->_nanos += rhs.nanos();
			return *this;
		}

		template<typename RT>
		CX_Time_t<TimeUnit>& operator-=(const CX_Time_t<RT>& rhs) {
			this->_nanos -= rhs.nanos();
			return *this;
		}

		/*! \brief Compares two times in the expected way. */
		template <typename RT>
		bool operator < (const CX_Time_t<RT>& rhs) const {
			return this->_nanos < rhs.nanos();
		}

		/*! \brief Compares two times in the expected way. */
		template <typename RT>
		bool operator <= (const CX_Time_t<RT>& rhs) const {
			return this->_nanos <= rhs.nanos();
		}

		/*! \brief Compares two times in the expected way. */
		template <typename RT>
		bool operator >(const CX_Time_t<RT>& rhs) const {
			return this->_nanos > rhs.nanos();
		}

		/*! \brief Compares two times in the expected way. */
		template <typename RT>
		bool operator >= (const CX_Time_t<RT>& rhs) const {
			return this->_nanos >= rhs.nanos();
		}

		/*! \brief Compares two times in the expected way. */
		template <typename RT>
		bool operator == (const CX_Time_t<RT>& rhs) const {
			return this->_nanos == rhs.nanos();
		}

		/*! \brief Compares two times in the expected way. */
		template <typename RT>
		bool operator != (const CX_Time_t<RT>& rhs) const {
			return this->_nanos != rhs.nanos();
		}

		/*! \brief Get the minimum time value that can be represented with this class. */
		static CX_Time_t<TimeUnit> min(void) {
			CX_Time_t<TimeUnit> t(0);
			t._nanos = std::numeric_limits<long long>::min();
			return t;
		}

		/*! \brief Get the maximum time value that can be represented with this class. */
		static CX_Time_t<TimeUnit> max(void) {
			CX_Time_t<TimeUnit> t(0);
			t._nanos = std::numeric_limits<long long>::max();
			return t;
		}

		/*! This function calculates the sample standard deviation for a vector of time values. */
		static CX_Time_t<TimeUnit> standardDeviation(std::vector<CX_Time_t<TimeUnit>> vals) {

			//because gcc is complete fucking garbage and cannot do fucking easy shit like function lookup within
			//a namespace, this function has to be written out completely and can't use Util::var. It could be:
			//std::vector<double> timeValues(vals.size());
			//for (unsigned int i = 0; i < vals.size(); i++) {
			//	timeValues[i] = vals[i].value();
			//}
			//return sqrt(Util::var(timeValues));

			//Implementation of single-pass variance: http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Incremental_Algorithm
			double mean = 0;
			double M2 = 0;

			for (unsigned int i = 0; i < vals.size(); i++) {
				double timeValue = vals[i].value();
				double delta = timeValue - mean;
				mean = mean + delta / (i + 1);
				M2 = M2 + delta*(timeValue - mean);
			}
			return sqrt(M2 / (vals.size() - 1));
		}

	private:
		long long _nanos;

	};

	/*! This is a standard stream operator for inserting a CX_Time_t into a std::ostream.
	\note If operator<< and operator>> are used to convert to/from a stream representation, you MUST use the same
	time units on both ends of the conversion. In addition, converting to/from string is not guaranteed to be a 
	lossless operation. */
	template <typename TimeUnit>
	std::ostream& operator<< (std::ostream& os, const CX_Time_t<TimeUnit>& t) {
		os << t.value(); //Assume sufficient precision to encode without losing nanos? Or set the pecision?
		return os;
	}

	/*! Stream extraction operator for CX_Time_t. */
	template <typename TimeUnit>
	std::istream& operator>> (std::istream& is, CX_Time_t<TimeUnit>& t) {
		double value;
		is >> value;
		t = CX_Time_t<TimeUnit>(value);
		return is;
	}
}
