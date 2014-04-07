#pragma once

#include "ofEvents.h"

namespace CX {
namespace Private {

	struct CX_MouseScrollEventArgs_t {
		CX_MouseScrollEventArgs_t(void) : x(0), y(0) {};
		CX_MouseScrollEventArgs_t(double x_, double y_) : x(x_), y(y_) {}
		double x;
		double y;
	};

	struct CX_KeyRepeatEventArgs_t {
		int key;
	};

	extern ofEvent<CX_MouseScrollEventArgs_t> scrollEvent;
	extern ofEvent<CX_KeyRepeatEventArgs_t> keyRepeatEvent;

}
}