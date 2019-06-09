#include "CX_Time_t.h"

namespace CX {

CX_TimeParts::CX_TimeParts(void) :
	sign(1),
	hours(0),
	minutes(0),
	seconds(0),
	milliseconds(0),
	microseconds(0),
	nanoseconds(0)
{}

std::string CX_TimeParts::toString(void) const {
	std::ostringstream oss;
	if (this->sign == -1) {
		oss << '-';
	}
	oss << this->hours << ":" <<
		_zps(this->minutes, 2) << ":" <<
		_zps(this->seconds, 2) << "." <<
		_zps(this->milliseconds, 3) << "." <<
		_zps(this->microseconds, 3) << "." <<
		_zps(this->nanoseconds, 3);
	return oss.str();
}

void CX_TimeParts::fromString(const std::string& str) {
	std::istringstream is(str);
	fromIstream(is);
}


void CX_TimeParts::fromIstream(std::istream& is) {

	if (is.peek() == '-') {
		is.ignore(1);
		this->sign = -1;
	}
	else {
		this->sign = +1;
	}

	is >> this->hours;
	is.ignore(1);
	is >> this->minutes;
	is.ignore(1);
	is >> this->seconds;
	is.ignore(1);
	is >> this->milliseconds;
	is.ignore(1);
	is >> this->microseconds;
	is.ignore(1);
	is >> this->nanoseconds;

	//std::string str(std::istreambuf_iterator<char>(is), {});
	//fromString(str);
}

/*!
Format %x where x is replaced by one of:

H: Hours
M: Minutes
S: Seconds

m: Milliseconds
u: Microseconds
n: Nanoseconds

Sign placed at front only if negative.

The first number is not zero padded. 
Hours are never zero padded (there is no day, so hours can be greater than 24).
*/
std::string CX_TimeParts::toFormattedString(std::string fmt) const {

	auto formatNextNumber = [this](char c, bool zp) -> std::string {
		switch (c) {
		case 'H': return _zps(this->hours, 0);
		case 'M': return _zps(this->minutes, zp ? 2 : 0);
		case 'S': return _zps(this->seconds, zp ? 2 : 0);
		case 'm': return _zps(this->milliseconds, zp ? 3 : 0);
		case 'u': return _zps(this->microseconds, zp ? 3 : 0);
		case 'n': return _zps(this->nanoseconds, zp ? 3 : 0);
		//case '-': return (this->sign > 0) ? "" : "-";
		//case '+': return (this->sign > 0) ? "+" : "-";
		}
		return "";
	};

	std::ostringstream oss;
	if (this->sign == -1) {
		oss << '-';
	}
	bool zeroPad = false;
	bool formatNextChar = false;
	for (const char& c : fmt) {
		if (c == '%') {
			formatNextChar = true;
			continue;
		}
		if (formatNextChar) {
			oss << formatNextNumber(c, zeroPad);
			zeroPad = true;
			formatNextChar = false;
		} else {
			oss << c;
		}
	}

	return oss.str();
}


// zero-padded string
std::string CX_TimeParts::_zps(int i, int digits) {
	std::ostringstream oss;
	if (digits > 0) {
		oss << std::setfill('0') << std::setw(digits);
	}
	oss << i;
	return oss.str();
}

std::ostream& operator<<(std::ostream& os, const CX_TimeParts& pt) {
	os << pt.toString();
	return os;
}

std::istream& operator>>(std::istream& is, CX_TimeParts& pt) {
	pt.fromIstream(is);
	return is;
}

} // namespace CX