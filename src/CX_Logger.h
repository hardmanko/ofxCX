#ifndef _CX_DEFERRED_LOGGER_H_
#define _CX_DEFERRED_LOGGER_H_

#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>



#include "ofUtils.h"
#include "ofFileUtils.h"
#include "ofEvents.h"
#include "ofLog.h"

#include "CX_Clock.h"
#include "CX_LoggerChannel.h"

//using namespace std; //TODO: Remove

namespace CX {

	//class CX_LoggerChannel;
	//struct CX_ofLogMessageEventData_t;


	/*! \enum LogLevel
	Log levels for log messages. Depending on the log level chosen, the name of the level will be printed before the message.
	Depending on the settings set using level(), levelForConsole(), or levelForFile(), if the log level of a message is below
	the level set for the module or logging target it will not be printed. For example, if LOG_ERROR is the level for the console
	and LOG_NOTICE is the level for the module "test", then messages logged to the "test" module will be completely ignored if
	at verbose level (because of the module setting) and will not be printed to the console if they are below the error level.
	*/
	//These rely on ordering; do not change it.
	enum class LogLevel {
		LOG_ALL, //This is functionally identical to LOG_VERBOSE, but it is more obvious what it does
		LOG_VERBOSE,
		LOG_NOTICE,
		LOG_WARNING,
		LOG_ERROR,
		LOG_FATAL_ERROR,
		LOG_NONE
	};

	//Forward declarations of internally used structs
	struct LogMessage;
	struct LoggerTargetInfo;

	struct MessageFlushData {
		MessageFlushData (string message_, LogLevel level_, string module_) :
			message(message_),
			level(level_),
			module(module_)
		{}
		string message;
		LogLevel level;
		string module;
	};

	class CX_Logger {
	public:

		CX_Logger (void);
		~CX_Logger (void);

		stringstream& verbose (std::string module = "");
		stringstream& notice(std::string module = "");
		stringstream& warning(std::string module = "");
		stringstream& error(std::string module = "");
		stringstream& fatalError(std::string module = "");

		void level(LogLevel level, std::string module = "");
		void levelForConsole (LogLevel level);
		void levelForFile(LogLevel level, std::string filename = "CX_DEFERRED_LOGGER_DEFAULT");
		void levelForAllModules(LogLevel level);

		void flush (void); //BLOCKING

		void timestamps(bool logTimestamps, std::string format = "%H:%M:%S.%i");

		//ofEvent<LogEventData> messageFlushEvent;

		void setMessageFlushCallback (std::function<void(MessageFlushData&)> f);

		void captureOFLogMessages(void) {
			ofSetLoggerChannel(ofPtr<ofBaseLoggerChannel>(dynamic_cast<ofBaseLoggerChannel*>(&this->_ofLoggerChannel)));
		}

	private:

		vector<LoggerTargetInfo> _targetInfo;
		map<std::string, LogLevel> _moduleLogLevels;
		vector<LogMessage> _messageQueue;

		std::stringstream& _log(LogLevel level, std::string module);
		std::string _getLogLevelName(LogLevel level);

		std::function<void(MessageFlushData&)> _flushCallback;

		bool _logTimestamps;
		std::string _timestampFormat;

		LogLevel _defaultLogLevel;

		CX_LoggerChannel _ofLoggerChannel;
		void _loggerChannelEventHandler(CX_ofLogMessageEventData_t& md);
	};

	namespace Instances {
		extern CX_Logger Log;
	}

	namespace Private {
		//extern CX_LoggerChannel ofLoggerChannel;

	}
	

}

/*	
Log.verbose() << "TMI!";

Log.level(CX::LogLevel::ERROR, "Invisible");
Log.warning("Invisible") << "You shouldn't see this";
Log.error("Invisible") << "There should not be a warning from the Invisible module above this.";
Log.fatalError("Invisible") << "You should see this message.";

Log.notice("CX_RandomNumberGenerator") << "This is not a random number: " << 57;

Log.warning("Dense") << "Too dense to sense!";

Log.error() << "This is a general purpose error";

Log.fatalError("CX_Logger") << "Too much input!";

//This is disgusting and totally not supported
stringstream* ss = &Log.warning("pointer");
*ss << "ook ";
*ss << 5;

Log.levelForConsole(CX::LogLevel::ERROR);

Log.flush();
*/

#endif //_CX_DEFERRED_LOGGER_H_