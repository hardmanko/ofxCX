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

	class CX_Events {
	public:
		CX_Events(void);
		~CX_Events (void);

		void setup (void);

		ofEvent<CX_MouseScrollEventArgs_t> scrollEvent;
	};
	
	CX_Events& getEvents(void);
}
}