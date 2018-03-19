#include "CX_Time_t.h"

namespace CX {

	PartitionedTime::PartitionedTime(void) :
		sign(1),
		hours(0),
		minutes(0),
		seconds(0),
		milliseconds(0),
		microseconds(0),
		nanoseconds(0)
	{}

	std::string PartitionedTime::getString(void) const {
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

	void PartitionedTime::setFromString(const std::string& str) {
		std::istringstream iss(str);

		if (iss.peek() == '-') {
			iss.ignore(1);
			this->sign = -1;
		} else {
			this->sign = +1;
		}

		iss >> this->hours;
		iss.ignore(1);
		iss >> this->minutes;
		iss.ignore(1);
		iss >> this->seconds;
		iss.ignore(1);
		iss >> this->milliseconds;
		iss.ignore(1);
		iss >> this->microseconds;
		iss.ignore(1);
		iss >> this->nanoseconds;
	}

	void PartitionedTime::setFromIstream(std::istream& is) {
		std::string str(std::istreambuf_iterator<char>(is), {});
		setFromString(str);
	}

	// zero-padded string
	std::string PartitionedTime::_zps(int i, int digits) {
		std::ostringstream oss;
		oss << std::setfill('0') << std::setw(digits) << i;
		return oss.str();
	}

	std::ostream& operator<<(std::ostream& os, const PartitionedTime& pt) {
		os << pt.getString();
		return os;
	}

	std::istream& operator >> (std::istream& is, PartitionedTime& pt) {
		pt.setFromIstream(is);
		return is;
	}

}