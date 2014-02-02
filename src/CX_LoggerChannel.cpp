#include "CX_LoggerChannel.h"

using namespace CX;
using namespace std;

void CX_LoggerChannel::log(ofLogLevel level, const string & module, const string & message) {
	CX_ofLogMessageEventData_t md;
	md.level = level;
	md.module = module;
	md.message = message;
	ofNotifyEvent(this->messageLoggedEvent, md);
}

void CX_LoggerChannel::log(ofLogLevel level, const string & module, const char* format, ...) {
	va_list args;
	va_start(args, format);
	this->log(level, module, format, args);
	va_end(args);
}

void CX_LoggerChannel::log(ofLogLevel level, const string & module, const char* format, va_list args) {
	bool failed = true;
	int bufferSize = 256;
	do {
		
		char *buffer = new char[bufferSize];
		int success = vsnprintf(buffer, bufferSize - 1, format, args);
		if (success > 0 && success < bufferSize) {
			failed = false;

			this->log(level, module, string(buffer));

		} else {
			bufferSize *= 4;
			if (bufferSize > 9000) {
				this->log(ofLogLevel::OF_LOG_ERROR, "CX_LoggerChannel", "Could not convert formatted arguments.");
			}
		}

		delete[] buffer;

	} while (failed);
}