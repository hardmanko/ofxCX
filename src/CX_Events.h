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
		CX_KeyRepeatEventArgs_t(void) :
			key(0),
			keycode(0),
			scancode(0),
			codepoint(0)
		{}

		int key;
		int keycode;
		int scancode;
		unsigned int codepoint;
	};

	class CX_Events {
	public:
		ofEvent<CX_MouseScrollEventArgs_t> scrollEvent;
		ofEvent<CX_KeyRepeatEventArgs_t> keyRepeatEvent;
	};

	CX_Events& getEvents(void);

}
}