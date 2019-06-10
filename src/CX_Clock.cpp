#include "CX_Clock.h"

#include "ofUtils.cpp" //for whatever reason, the function I need (ofGetMonotonicTime) is not in the header, only in the .cpp

#include "CX_Private.h"
#include "CX_InputManager.h" // Instances::Input
#include "CX_Logger.h" // Instances::Log

namespace CX {

CX_Clock CX::Instances::Clock;

CX_Clock::CX_Clock(void) {
	_regularEvent.enabled = false;
	_regularEvent.period = CX_Millis(10);
}

/*! Set up the CX_Clock with the given clock implementation or choose the best available implementation.

Instances of CX_Clock are not constructed in a usable state. This function must be called before using a
CX_Clock instance.

\param impl A `shared_ptr` to the clock implementation to use. If `nullptr`, the clock implementation will
be chosen using `chooseBestClockImplementation()` with various values for `excludeUnstable` and `excludeWorseThanMs`
until an implementation is chosen.
\param resetStartTime If `true`, the experiment start time will be reset.
\param samples Only used if `impl == nullptr`. Passed to the `samples` argument of `chooseBestClockImplementation()`.

\return `true` if setup was successful, `false` otherwise.

\note This function is called for `CX::Instances::Clock` with `impl = nullptr` during CX initialization.
*/
bool CX_Clock::setup(std::shared_ptr<CX_BaseClockInterface> impl, bool resetStartTime, unsigned int samples) {

	if (impl == nullptr) {

		// During setup, use a dummy clock impl
		setImplementation<CX_DummyClock>();

		std::string warning = "";
		std::string baseStr = "The chosen clock implementation ";
		std::string monotonicWF = "is non-monotonic/not steady (may jump forward or back in time)";
		std::string worseMsWF = "may have precision worse than 1 ms";

		auto clockImpl = CX::CX_Clock::chooseBestClockImplementation(samples, true, true, false);

		if (clockImpl.first == nullptr) {
			clockImpl = CX::CX_Clock::chooseBestClockImplementation(samples, false, true, false); // drop steady
			warning = baseStr + monotonicWF + ". ";
		}
		if (clockImpl.first == nullptr) {
			clockImpl = CX::CX_Clock::chooseBestClockImplementation(samples, true, false, false); // drop steady, but keep sub ms
			warning = baseStr + worseMsWF + ". ";;
		}
		if (clockImpl.first == nullptr) {
			clockImpl = CX::CX_Clock::chooseBestClockImplementation(samples, false, false, false); // exclude nothing
			warning = baseStr + monotonicWF + " and " + worseMsWF + ". ";
		}

		impl = clockImpl.first;

		if (impl == nullptr) {
			CX::Instances::Log.error("CX_Clock") << "setup(): No clock implementation could be chosen.";
			return false;
		}

		CX::Instances::Log.notice("CX_Clock") << "setup(): The chosen clock implementation is " << impl->getName() << ".";

		if (warning != "") {
			CX::Instances::Log.warning("CX_Clock") << "setup(): " << warning;
		}

	}

	setImplementation(impl);

	if (resetStartTime) {
		resetExperimentStartTime();
	}

	return true;
}



/*! Set the underlying clock implementation used by this instance of CX_Clock. 

You would use this function if the clock implementation chosen during setup has insufficient 
precision on your system. This is unlikely because several clock implementations are checked
during setup and the best one chosen.

The clock implementations built into CX are `CX_ofMonotonicTimeClock`, 
`CX_WIN32_PerformanceCounterClock` (Windows only), and `CX_StdClockWrapper`, which wraps
clocks from the `std::chrono` namespace, including `steady_clock`, `high_resolution_clock`, 
and `system_clock`.
You can use any clock that implements CX_BaseClockInterface, including implementing your own 
clock. 

\note This function resets the experiment start time of `impl`, but does not
reset the experiment start time date/time string.

\param impl A shared_ptr to a clock implementing `CX_BaseClockInterface` (see example).

\code{.cpp}

// Example with CX_ofMonotonicTimeClock
auto implA = std::make_shared< CX_ofMonotonicTimeClock >();
Clock.setImplementation(implA);

// Example using CX_StdClockWrapper with steady_clock as the wrapped standard clock
auto implB = std::make_shared< CX_StdClockWrapper<std::chrono::steady_clock> >();
Clock.setImplementation(implB);

\endcode
*/
void CX_Clock::setImplementation(std::shared_ptr<CX_BaseClockInterface> impl) {
	_impl = impl;
	_impl->resetStartTime();
}

std::shared_ptr<CX_BaseClockInterface> CX_Clock::getImplementation(void) const {
	return _impl;
}

/*! If for some reason you have a long setup period before the experiment proper
starts, you could call this function so that the values returned by CX_Clock::now()
will count up from 0 starting from when this function was called.

This function is not thread safe: If called while the clock is being used in another
thread to get the current time while the start time is being updated, it is possible
for some timestamps to be corrupted and display invalid times. The program will not crash.

This function also resets the experiment start date/time retrieved with getExperimentStartDateTimeString().
*/
void CX_Clock::resetExperimentStartTime(void) {
	_experimentStartTime = std::time(nullptr);

	if (_impl) {
		_impl->resetStartTime();
	}
}

/*! Returns the current time relative to the start of the experiment in milliseconds.
The start of the experiment is defined by default as when the CX_Clock instance named `CX::Instances::Clock`
is set up during the beginning of program execution. See also `resetExperimentStartTime()`.

\return A `CX_Millis` object containing the time.

\note See \ref CX::cxTick_t for calculations showing the amount of time that can be stored
by a `CX_Millis` object.

\note This cannot be converted to current date/time in any meaningful way. Use getDateTimeString() for that.*/
CX_Millis CX_Clock::now(void) const {
	return CX_Nanos(_impl->nanos());
}

/*! Sleeps for the requested period of time. This can be somewhat
imprecise because it requests a specific sleep duration from the operating system,
but the operating system may not provide the exact sleep time.

This function is effectively a static function of the CX_Clock class.

If CX is running on a single-core computer, it is a good idea to call `Clock.sleep(0)`
in wait loops when waiting on input or other slow response. Giving the argument of 0
causes the CX experiment to yield, which allows CPU time to be used for other programs 
running on the computer. It is not neccessary or even a good idea to call `sleep(0)`
in tight wait loops that are expected to wait only a few milliseconds total.

\param t The requested sleep duration. If 0, the thread yields rather than sleeping.
*/
void CX_Clock::sleep(CX_Millis t) const {
	if (t.nanos() == 0) {
		std::this_thread::yield();
	} else {
		std::this_thread::sleep_for(std::chrono::nanoseconds(t.nanos()));
	}
}

/*
void CX_Clock::interruptedSleep(CX_Millis total, CX_Millis unit, std::function<void(void)> fun) const {
	CX_Millis endTime = this->now() + total;
	if (unit < total) {
		unit = total;
	}

	while (this->now() < endTime) {

		Instances::Input.pollEvents();

		if (fun != nullptr) {
			fun();
		}

		this->sleep(unit);

	};

}

void CX_Clock::interruptedDelay(CX_Millis total, CX_Millis unit, std::function<void(void)> fun) const {
	CX_Millis endTime = this->now() + total;

	while (this->now() < endTime) {

		Instances::Input.pollEvents();

		if (fun != nullptr) {
			fun();
		}

		this->delay(unit);

	};
}
*/

/*! Blocks in a tight spinloop for the requested period of time. This is likely more
precise than CX_Clock::sleep() because it does not give up control to the operating
system, but it wastes resources because it just sits in a spinloop for the requested
duration. 

This function is effectively a static function of the CX_Clock class.

\param t The requested delay duration.
*/
void CX_Clock::delay(CX_Millis t) const {
	CX_Millis endTime = this->now() + t;
	while (this->now() < endTime)
		;
}

// Helper function for time
std::string CX_Clock::_formatTime(const std::string& format, const std::time_t* time) const {

	char output[256];
	std::size_t written = std::strftime(output, 256, format.c_str(), std::localtime(time));

	std::string rval = "";
	if (written > 0) {
		rval = std::string(output);
	}

	return rval;
}

/*! Get a string representing the date/time of the start of the experiment encoded according to a format.
\param format See getDateTimeString() for the definition of the format. */
std::string CX_Clock::getExperimentStartDateTimeString(const std::string& format) const {

	return _formatTime(format, &_experimentStartTime);
}

/*! Returns a string containing the local time encoded according to a format string.

The format string `"%Y/%m/%d %H:%M %p"` gives `"year/month/day 24HourClock:minute (am|pm)"` with some zero-padding for most things.
The default uses hyphens so that the timestamps can be used as filenames.

\param format The format string is passed to `std::strftime`. See documentation for that function
for information about the format string: https://en.cppreference.com/w/cpp/chrono/c/strftime
*/
std::string CX_Clock::getDateTimeString(const std::string& format) const {

	std::time_t localTime = std::time(nullptr);
	return _formatTime(format, &localTime);
}

void CX_Clock::enableRegularEvent(bool enable) {
	std::lock_guard<std::recursive_mutex> lock(_regularEvent.mutex);

	if (enable) {
		if (_regularEvent.enabled) {
			return;
		} else {
			_regularEvent.enabled = true; // enable first
			_regularEvent.thread = std::thread(std::bind(&CX_Clock::_regularEventThreadFunction, this));
		}
	} else {
		_regularEvent.enabled = false;
	}
}

bool CX_Clock::isRegularEventEnabled(void) {
	std::lock_guard<std::recursive_mutex> lock(_regularEvent.mutex);
	return _regularEvent.enabled;
}

void CX_Clock::setRegularEventPeriod(CX_Millis period) {
	std::lock_guard<std::recursive_mutex> lock(_regularEvent.mutex);
	_regularEvent.period = period;
}

CX_Millis CX_Clock::getRegularEventPeriod(void) {
	std::lock_guard<std::recursive_mutex> lock(_regularEvent.mutex);
	return _regularEvent.period;
}

void CX_Clock::_regularEventThreadFunction(void) {

	// The way to make this even more fancy is to use a PID
	// controller (or somesuch)

	_regularEvent.mutex.lock();

	bool threadRunning = _regularEvent.enabled;
	CX_Millis period = _regularEvent.period;
	
	_regularEvent.mutex.unlock();

	CX_Millis nextWakeTarget = this->now() + period;
	CX_Millis nextPeriodAdjustment = 0;
	

	while (threadRunning) {

		_regularEvent.mutex.lock();
		threadRunning = _regularEvent.enabled;
		period = _regularEvent.period;
		_regularEvent.mutex.unlock();

		if (!threadRunning) {
			break;
		}

		this->sleep(period + nextPeriodAdjustment);

		CX_Millis wakeTime = this->now();

		this->regularEvent.notify();

		if (nextWakeTarget < CX_Millis(0)) {
			nextWakeTarget = wakeTime;
		}
		CX_Millis oversleep = wakeTime - nextWakeTarget;

		nextPeriodAdjustment = std::min(-oversleep, period / 5); // Don't add more than 1/5th of a period. or 0 for don't add ever.

		CX_Millis processingTime = this->now() - wakeTime; // This seems like overkill, but allows users to do processing during event notification

		nextWakeTarget = wakeTime + period + nextPeriodAdjustment - processingTime;

	}

}

/*! Tests the precision, with `testImplPrecision()`, of all of the clock implementations that are 
built-in to CX and chooses the best one on the basis of the following criteria:

1. If `excludeUnstable == true`, clock implementations that are unstable/not monotonic are excluded.
2. If `excludeWorseThanMs == true`, clock implmentations with precision worse than 1 ms are excluded.
3. Of the remaining clock implementations, the one with the lowest mean precision for non-zero length
time intervals.

\note This function is used during CX initialization to select the best clock implementation to use
for the given session. This means that different sessions on the same computer may use different
clock implementations, although there can't be much difference between those implementations if
different implementations are chosen for different sessions.

\param samples Number of time intervals samples to take per clock implementation. 
More samples should give better estimates of precision. 
\param excludeUnstable If `true`, clock implementations that are unstable/non-monotonic are excluded
from consideration. Unstable clocks may go back in time or jump forward.
\param excludeWorseThanMs If `true`, clock implementations that have worse than 1 millisecond precision 
are excluded.
\param log If `true`, results from all tested implementations are logged.

\return A pair where the first value is a `shared_ptr` to the chosen clock implementation (may be 
`nullptr` if no implementation is chosen) and the second value is the precision test results for 
the chosen implementation. See example.

\code{.cpp}

// Choose the best built-in clock impl.
auto bestClockImpl = CX_Clock::chooseBestClockImplementation(100000, true, true, true);

// Set the selected implementation as the one used by Clock.
Clock.setImplementation(bestClockImpl.first);

// Examine precision results
CX_Clock::PrecisionTestResults precRes = bestClockImpl.second;
Log.notice() << precRes.summaryString;
Log.flush();

\endcode

*/
std::pair<std::shared_ptr<CX_BaseClockInterface>, CX_Clock::PrecisionTestResults> 
	CX_Clock::chooseBestClockImplementation(unsigned int samples, bool excludeUnstable, bool excludeWorseThanMs, bool log) {

	typedef std::pair<std::shared_ptr<CX_BaseClockInterface>, CX_Clock::PrecisionTestResults> TestRes;

	std::map<std::string, TestRes> results;

	{
		TestRes res;

		res.first = std::make_shared<CX_StdClockWrapper<std::chrono::high_resolution_clock>>();
		res.second = CX_Clock::testImplPrecision(res.first, samples);

		results["high_resolution_clock (" + res.first->getName() + ")"] = res;
	}

	{
		TestRes res;

		res.first = std::make_shared<CX_StdClockWrapper<std::chrono::system_clock>>();
		res.second = CX_Clock::testImplPrecision(res.first, samples);

		results["system_clock (" + res.first->getName() + ")"] = res;
	}

	{
		TestRes res;

		res.first = std::make_shared<CX_StdClockWrapper<std::chrono::steady_clock>>();
		res.second = CX_Clock::testImplPrecision(res.first, samples);

		results["steady_clock (" + res.first->getName() + ")"] = res;
	}

#ifdef TARGET_WIN32
	{
		TestRes res;

		res.first = std::make_shared<CX_WIN32_PerformanceCounterClock>();
		res.second = CX_Clock::testImplPrecision(res.first, samples);

		results[res.first->getName()] = res;
	}
#endif

#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 9 && OF_VERSION_PATCH >= 0
	{
		TestRes res;

		res.first = std::make_shared<CX_ofMonotonicTimeClock>();
		res.second = CX_Clock::testImplPrecision(res.first, samples);

		results[res.first->getName()] = res;
	}
#endif

	// Select best based on 1) monotonicity and 2) mean nonnzero latency
	TestRes bestImpl;
	bestImpl.first = nullptr; // Explicitly nullptr unless updated
	bestImpl.second.withoutZeros.mean = CX_Millis::max();

	for (const auto& res : results) {
		if (excludeUnstable && !res.second.second.isMonotonic) {
			continue; // ignore unstable clocks
		}

		if (excludeWorseThanMs && res.second.second.precisionWorseThanMs) {
			continue;
		}

		if (res.second.second.withoutZeros.mean < bestImpl.second.withoutZeros.mean) {
			bestImpl = res.second;
		}
	}


	// Print all results
	if (log) {
		std::ostringstream oss;
		oss << std::endl << std::endl << "Results for all built-in clock implementations." << std::endl  << std::endl;

		for (auto& res : results) {
			oss << std::string(20, '#') << std::endl << std::endl;
			oss << res.second.second.summaryString << std::endl;
		}

		oss << std::string(20, '#') << std::endl << std::endl;
		oss << "Chosen implementation: " << bestImpl.first->getName() << std::endl;

		Instances::Log.notice("CX_Clock") << "chooseBestClockImplementation(): " << oss.str();
	}

	return bestImpl;
}

/*! This function tests the precision of the clock implementation used by this instance of CX_Clock.
The results are computer-specific.
If the precision of the clock is worse than millisecond accuracy, a warning is logged including
information about the actual precision of the clock.

The precision is estimated by sampling `samples` pairs of time points sampled immediately adjacent
to one another. The difference between each pair of time points is calculated, giving `samples`
time differences. Depending on the clock implementation that is in use, the counter for the implementation
will tick forward at a certain rate. This testing code can check the current time more quickly than
the clock implementation's counter moves forward. This means that many of the time intervals that are
sampled have the value 0 because the clock implementation did not tick forward between the first and
second time points at which the time was sampled.

The non-zero time intervals can be used to learn about the precision of the clock. Specifically,
the lowest non-zero interval is a reasonable estimate of the minimum time step that can be taken
by the clock implementation.

If the clock implementation has low precision (long intervals between updating its internal counter),
this will be manifested as a large proportion of zero-length intervals and a smaller proportion of 
longer intervals. For reference, even for fairly precise clocks (e.g. the internal counter ticks 
every 1/3 of a microsecond), the proportion of zero-length intervals can be on the order of 80%. 

Depending on the number of samples, this function may be blocking. See \ref blockingCode.

\note See `setImplementation()` for a way to set the clock implementation used by the `CX_Clock`.

\note Do not perform precision tests if using builds compiled for debugging; make sure you build
for release when performing precision tests.

\param impl A `shared_ptr` to the clock implementation to test. See `setImplementation()` for an 
example of how to create a shared pointer.
\param samples Number of time intervals samples to take. More samples should give better estimates of precision.
\param percentiles A vector of percentiles (as proportions between 0 and 1) at which to find
quantiles of the time samples. If an empty vector is provided, defaults will be used.

\return A CX_Clock::PrecisionTestResults struct containing some information about the precision of the clock.
*/
CX_Clock::PrecisionTestResults CX_Clock::testImplPrecision(std::shared_ptr<CX_BaseClockInterface> impl, unsigned int samples, std::vector<double> percentiles) {

	std::vector<cxTick_t> intervals(samples);

	for (unsigned int i = 0; i < intervals.size(); i++) {
		//Get two timestamps with as little code in between as possible.
		cxTick_t t1 = impl->nanos();
		cxTick_t t2 = impl->nanos();

		intervals[i] = t2 - t1;
	}

	PrecisionTestResults rval;

	rval.isMonotonic = impl->isMonotonic();
	
	
	if (percentiles.empty()) {
		percentiles = { 0, 0.25, 0.5, 0.95, 0.99, 0.999, 0.9999, 0.99999, 1 };
	}
	rval.percentiles = Util::clamp<double>(percentiles, 0, 1);

	rval.withZeros.mean = CX_Nanos(Util::mean(intervals));
	std::vector<cxTick_t> qs = Util::quantile(intervals, rval.percentiles, true);
	for (cxTick_t q : qs) {
		rval.withZeros.quantiles.push_back(CX_Nanos(q));
	}

	std::vector<cxTick_t> nonZeroIntervals;
	for (unsigned int i = 0; i < intervals.size(); i++) {
		if (intervals[i] > 0) {
			nonZeroIntervals.push_back(intervals[i]);
		}
	}

	rval.withoutZeros.mean = CX_Nanos(Util::mean(nonZeroIntervals));
	qs = Util::quantile(nonZeroIntervals, rval.percentiles, true);
	for (cxTick_t q : qs) {
		rval.withoutZeros.quantiles.push_back(CX_Nanos(q));
	}


	// Summary string section
	unsigned int colw = 16;
	char fillc = ' ';
	unsigned int dprec = 3;
	auto pads = [&](std::string s) -> std::string {
		std::ostringstream os;
		os << std::setfill(fillc) << std::setw(colw) << s;
		return os.str();
	};

	auto padd = [&](double d) -> std::string {
		std::ostringstream os;
		os << std::fixed << std::setprecision(dprec) << std::setfill(fillc) << std::setw(colw) << d;
		return os.str();
	};

	std::ostringstream oss;

	oss << "Results for clock implementation: " << impl->getName() << std::endl;

	oss << "Stable/monotonic: " << (impl->isMonotonic() == false ? "false" : "true") << 
		(!impl->isMonotonic() ? " <<< Warning: Implementation is not monotonic >>>" : "") << std::endl;

	CX_Nanos minNonzero(Util::min(nonZeroIntervals));
	rval.precisionWorseThanMs = minNonzero > CX_Millis(1);
	
	oss << "Clock precision (minimum nonzero step size): " << minNonzero.micros() << " microseconds. ";

	if (rval.precisionWorseThanMs) {
		std::ostringstream wss;

		wss << "The precision of the clock implementation \"" << impl->getName() << "\" appears to be worse than "
			"millisecond precision. Observed tick period of the clock implmentation is " << minNonzero.millis() << " milliseconds. "
			"See CX_Clock::setImplementation() for information about choosing a clock implementation." << std::endl << rval.summaryString;

		oss << "Warning: " << wss.str();

		CX::Instances::Log.warning("CX_Clock") << wss.str();
	}

	oss << std::endl;

	oss << "Times in the table are in microseconds." << std::endl << std::endl;

	oss << pads("Statistic") << pads("With 0s") << pads("Without 0s") << std::endl;
	oss << std::string(colw * 3, '-') << std::endl;

	unsigned int zeroCount = std::count(intervals.begin(), intervals.end(), 0);
	double zeroPercent = (double)zeroCount * 100.0 / (double)samples;
	oss << pads("Percent == 0:") << ofToString(zeroPercent, 1, colw - 1, fillc) + "%" << pads("0%") << std::endl;

	oss << pads("Mean:") << padd(rval.withZeros.mean.micros()) << padd(rval.withoutZeros.mean.micros()) << std::endl;

	unsigned int baseRows = 2;
	for (unsigned int i = 0; i < rval.percentiles.size(); i++) {
		oss << ofToString(rval.percentiles[i] * 100, dprec, colw - 2, fillc) << "%:" <<
			padd(rval.withZeros.quantiles[i].micros()) <<
			padd(rval.withoutZeros.quantiles[i].micros()) << std::endl;
	}

	rval.summaryString = oss.str();

	return rval;
}


///////////////////////////
// Clock implementations //
///////////////////////////

#define CX_NANOS_PER_SECOND 1000000000LL

#if OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR == 9 && OF_VERSION_PATCH >= 0

cxTick_t CX_ofMonotonicTimeClock::nanos(void) const {
	uint64_t seconds;
	uint64_t nanos;
	ofGetMonotonicTime(seconds, nanos);
	seconds -= _startTime.seconds;
	nanos -= _startTime.nanos;

	return (cxTick_t)(seconds * CX_NANOS_PER_SECOND + nanos);
}

void CX_ofMonotonicTimeClock::resetStartTime(void) {
	ofGetMonotonicTime(_startTime.seconds, _startTime.nanos);
}
#endif


#ifdef TARGET_WIN32

#include "Windows.h"

CX_WIN32_PerformanceCounterClock::CX_WIN32_PerformanceCounterClock(void) {
	_resetFrequency();
	resetStartTime();
}

//This only has at best 1 microsecond precision.
cxTick_t CX_WIN32_PerformanceCounterClock::nanos(void) const {
	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);

	cxTick_t adjustedCount = count.QuadPart - _startTime;
	cxTick_t seconds = adjustedCount / _frequency;

	//Note that if _frequency is greater than or equal to 10 GHz (or just slightly lower), this multiplication will overflow
	//before reaching the division.
	cxTick_t nanos = ((adjustedCount % _frequency) * CX_NANOS_PER_SECOND) / _frequency;

	return (seconds * CX_NANOS_PER_SECOND) + nanos;
}

void CX_WIN32_PerformanceCounterClock::resetStartTime(void) {
	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);
	_startTime = count.QuadPart;
}

void CX_WIN32_PerformanceCounterClock::setStartTime(cxTick_t ticks) {
	_startTime = ticks;
}

void CX_WIN32_PerformanceCounterClock::_resetFrequency(void) {
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	_frequency = li.QuadPart;
}

#endif //TARGET_WIN32

} //namespace CX
