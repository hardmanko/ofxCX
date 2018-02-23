#pragma once

#include <chrono>
#include <vector>

namespace CX {

	/*! The underlying time store for CX_Time_t (e.g. CX_Millis), which stores time in nanoseconds. 
	CX_Time_t can store time differences, which can be negative, so cxTick_t must be a signed int.
	
	cxTick_t is defined as a signed 64 bit integer (int64_t), which can store 2^63 nanoseconds before
	rolling over to a negative value. How long is that? 
	2^63 nanoseconds * 1 second / 10^9 nanoseconds = 9.2 * 10^9 (9.2 billion) seconds 
	9.2 * 10^9 seconds / 60 (secs/min) / 60 (mins/hr) / 24 (hrs / day) / 365 (days/year) = 292.47 years
	*/
	typedef int64_t cxTick_t;

	template <typename T> class CX_Time_t;

	typedef CX_Time_t<std::ratio<3600, 1> > CX_Hours; //!< Hours.
	typedef CX_Time_t<std::ratio<60, 1> > CX_Minutes; //!< Minutes.
	typedef CX_Time_t<std::ratio<1, 1> > CX_Seconds; //!< Seconds.
	typedef CX_Time_t<std::ratio<1, 1000> > CX_Millis; //!< Milliseconds.
	typedef CX_Time_t<std::ratio<1, 1000000> > CX_Micros; //!< Microseconds.
	typedef CX_Time_t<std::ratio<1, 1000000000> > CX_Nanos; //!< Nanoseconds.

	namespace Private {
		template<typename tOut, typename tIn, typename resultT>
		inline resultT convertTimeCount(resultT countIn) {
			double multiplier = ((double)tIn::num * tOut::den) / ((double)tIn::den * tOut::num);
			return resultT(countIn * multiplier);
		}

		template<>
		inline cxTick_t convertTimeCount<std::nano, std::nano, cxTick_t>(cxTick_t countIn) {
			return countIn;
		}

		template<>
		inline cxTick_t convertTimeCount<std::micro, std::micro, cxTick_t>(cxTick_t countIn) {
			return countIn;
		}

		template<>
		inline cxTick_t convertTimeCount<std::milli, std::milli, cxTick_t>(cxTick_t countIn) {
			return countIn;
		}
	}

	/*! The `CX_Time_t` class provides a convenient way to deal with time in different units, 
	mostly seconds and milliseconds for psychology experiments. Different time units are represented
	with different templated versions of the class, such as CX_Seconds and CX_Millis.
	
	For example, CX_Millis and CX_Seconds are two templated variations on the CX_Time_t class.
	This allows you to express that you want time in different units.
	\code{.cpp}
	CX_Millis hundredMillis = 100;
	CX_Seconds twoSecs = 2;
	\endcode
	Even though the times are expressed in different units, you can compare them in various ways.
	\code{.cpp}
	if (twoSecs > hundredMillis) {
		cout << "Two seconds is greater than 100 milliseconds" << endl;
	}
	CX_Seconds onePointNineSecs     = twoSecs - hundredMillis;
	CX_Millis nineteenHundredMillis = twoSecs - hundredMillis;
	if (onePointNineSecs == nineteenHundredMillis) {
		cout << "Units don't matter, only the underlying time representation." << endl;
	}

	// Get the underlying nanoseconds representation
	cxTick_t hundredMillionNanos = hundredMillis.nanos();
	\endcode
	
	The upside of this
	system is that although all functions in CX that take time can take time values in a variety of
	units. For example, CX_Clock::wait() takes CX_Millis as the time type so if you were to do
	\code{.cpp}
	Clock.wait(20);
	\endcode
	it would attempt to wait for 20 milliseconds. However, you could do
	\code{.cpp}
	Clock.wait(CX_Seconds(2));
	\endcode
	to wait for 2 seconds, if units of seconds are easier to think in for the given situation.
	
	CX_Time_t has at most nanosecond accuracy.
	See \ref CX::cxTick_t for calculations showing the amount of time that can be stored
	by a `CX_Time_t` object.
	The contents of any of the templated versions of CX_Time_t are all stored in nanoseconds, 
	so conversion between time types is lossless.

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
			int hours; //!< The hours component of the time.
			int minutes; //!< The minutes component of the time.
			int seconds; //!< The seconds component of the time.
			int milliseconds; //!< The milliseconds component of the time.
			int microseconds; //!< The microseconds component of the time.
			int nanoseconds; //!< The nanoseconds component of the time.
		};

		/*! Partitions a CX_Time_t into component parts containing the number of whole time units
		that are stored in the CX_Time_t. This is different from seconds(), millis(), etc., because
		those functions return the fractional part (e.g. 5.340 seconds) whereas this returns only
		whole numbers (e.g. 5 seconds and 340 milliseconds).
		\return A PartitionedTime struct containing whole number amounts of the components of the time.
		*/
		PartitionedTime getPartitionedTime(void) const {
			CX_Time_t<TimeUnit> t = *this; //Make a copy which is then modified.
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

		/*! Default constructor for CX_Time_t. */
		CX_Time_t(void) :
			_nanos(0)
		{}

		/*! Constructs a CX_Time_t with the specified time value. 
		\param t A time value with units interpreted depending on the TimeUnit 
		template argument for the instance of the class being constructed. 
		
		Example:
		\code{.cpp}
		CX_Minutes quarterHour(15); //Interpreted as 15 minutes
		CX_Seconds oneMinute(60); //Interpreted as 60 seconds
		\endcode
		*/
		CX_Time_t(double t) {
			_nanos = (cxTick_t)Private::convertTimeCount<std::nano, TimeUnit, double>(t);
		}

		/*! \copydoc CX_Time_t::CX_Time_t(double) */
		CX_Time_t(int t) {
			_nanos = Private::convertTimeCount<std::nano, TimeUnit, cxTick_t>(t);
		}

		/*! \copydoc CX_Time_t::CX_Time_t(double) */
		CX_Time_t(cxTick_t t) {
			_nanos = Private::convertTimeCount<std::nano, TimeUnit, cxTick_t>(t);
		}

		/*! Constructs a CX_Time_t based on another instance of a CX_Time_t. If the TimeUnit
		template parameter has a different value for `t` than for the CX_Time_t being constructed,
		it does not change the amount of time stored. For example, if `t` is a CX_Time_t<std::ratio<60, 1> >
		(i.e. CX_Minutes) containing 1 minute, and the CX_Time_t that is constructed will contain 1 minute 
		regardless of if that minute is thought of as 1/60 of an hour or 60,000,000 microseconds. */
		template <typename tArg>
		CX_Time_t(const CX_Time_t<tArg>& t) {
			this->_nanos = t.nanos();
		}

		/*! Get the numerical value of the time in units of the time type. For example, if
		you are using an instance of CX_Seconds, this will return the time value in seconds,
		including fractional seconds. */
		double value(void) const {
			return Private::convertTimeCount<TimeUnit, std::nano, double>(_nanos);
		}

		/*! \brief Get the time stored by this CX_Time_t in hours, including fractions of an hour. */
		double hours(void) const {
			return (double)_nanos / (1e9 * 60 * 60);
		}

		/*! \brief Get the time stored by this CX_Time_t in minutes, including fractions of a minute. */
		double minutes(void) const {
			return (double)_nanos / (1e9 * 60);
		}

		/*! \brief Get the time stored by this CX_Time_t in seconds, including fractions of a second. */
		double seconds(void) const {
			return (double)_nanos / 1e9;
		}

		/*! \brief Get the time stored by this CX_Time_t in milliseconds, including fractions of a millisecond. */
		double millis(void) const {
			return (double)_nanos / 1e6;
		}

		/*! \brief Get the time stored by this CX_Time_t in microseconds, including fractions of a microsecond. */
		double micros(void) const {
			return (double)_nanos / 1e3;
		}

		/*! \brief Get the time stored by this CX_Time_t in nanoseconds. */
		cxTick_t nanos(void) const {
			return _nanos;
		}

		/*! Converts the time to one of the standard library types, such as `std::chrono::milliseconds`.
		\tparam STT The return type you want, such as `std::chrono::milliseconds`.
		*/
		template <typename STT>
		STT getStdTimeType(void) const {
			std::chrono::nanoseconds nanos(this->_nanos);
			return std::chrono::duration_cast<STT>(nanos);
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
			return CX_Nanos(this->_nanos / rhs);
		}

		/*! \brief Multiplies a CX_Time_t by a unitless value, storing the result in the CX_Time_t.
		You cannot multiply a time by another time because that would result in units of time squared. */
		CX_Time_t<TimeUnit>& operator*=(double rhs) {
			this->_nanos *= rhs;
			return *this;
		}

		/*! \brief Adds a CX_Time_t to an existing CX_Time_t. 
		\param rhs The value to add. */
		template<typename RT>
		CX_Time_t<TimeUnit>& operator+=(const CX_Time_t<RT>& rhs) {
			this->_nanos += rhs.nanos();
			return *this;
		}

		/*! \brief Subtracts a CX_Time_t from an existing CX_Time_t. 
		\param rhs The value to subtract. */
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
			t._nanos = std::numeric_limits<cxTick_t>::min();
			return t;
		}

		/*! \brief Get the maximum time value that can be represented with this class. */
		static CX_Time_t<TimeUnit> max(void) {
			CX_Time_t<TimeUnit> t(0);
			t._nanos = std::numeric_limits<cxTick_t>::max();
			return t;
		}

		/*! Calculates the sample standard deviation for a vector of time values. */
		static CX_Time_t<TimeUnit> standardDeviation(std::vector<CX_Time_t<TimeUnit>> vals) {

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
		cxTick_t _nanos;

	};

	/*! Stream instertion operator for CX_Time_t. Stream insertion/extration may lose precision.
	\note If operator<< and operator>> are used to convert to/from a stream representation, you MUST use the same
	time units on both ends of the conversion.
	
	\code{.cpp}
	st::stringstream ss;
	CX_Seconds sec(60);

	ss << sec;
	cout << ss.str() << endl; //Ouputs the number 60.

	CX_Minutes min;
	ss >> min; //Reads in the 60 as 60 minutes.
	if (sec != min)) {
		cout << "The time values are now unequal." << endl;
	}

	//Better: Use the same type at both ends.
	CX_Seconds sec2;
	ss << sec;
	ss >> sec2;
	\endcode
	*/
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

	/*! \brief Multiplies a CX_Time_t with a numeric value, resulting in a CX_Time_t in the same units as `lhs`. */
	template<typename T>
	CX_Time_t<T> operator*(CX_Time_t<T> lhs, double rhs) {
		double n = lhs.nanos() * rhs;
		return CX_Nanos(n);
	}

	/*! \brief Multiplies a CX_Time_t with a numeric value, resulting in a CX_Time_t in the same units as `rhs`. */
	template<typename T>
	CX_Time_t<T> operator*(double lhs, CX_Time_t<T> rhs) {
		double n = rhs.nanos() * lhs;
		return CX_Nanos(n);
	}
}
