#pragma once

#include <stdio.h>
#include <stdarg.h>

#include "ofEvents.h"
#include "ofLog.h"

namespace CX {

	struct CX_ofLogMessageEventData_t {
		ofLogLevel level;
		std::string module;
		std::string message;
	};

	class CX_LoggerChannel : public ofBaseLoggerChannel {
	public:
		~CX_LoggerChannel(void) {};

		void log(ofLogLevel level, const std::string & module, const std::string & message);
		void log(ofLogLevel level, const std::string & module, const char* format, ...);
		void log(ofLogLevel level, const std::string & module, const char* format, va_list args);

		ofEvent<CX_ofLogMessageEventData_t> messageLoggedEvent;
	};

}