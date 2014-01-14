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

#include "CX_Clock.h"

//#undef ERROR //This is annoying and likely problematic.

using namespace std;

namespace CX {

	/*! \enum LogLevel
	Log levels for log messages. Depending on the log level chosen, the name of the level will be printed before the message.
	Depending on the settings set using level(), levelForConsole(), or levelForFile(), if the log level of a message is below
	the level set for the module or logging target it will not be printed. For example, if LOG_ERROR is the level for the console
	and LOG_NOTICE is the level for the module "test", then messages logged to the "test" module will be completely ignored if
	at verbose level (because of the module setting) and will not be printed to the console if they are below the error level.
	*/
	//These rely on ordering; do not change it.
	enum class LogLevel {
		LOG_ALL,
		LOG_VERBOSE,
		LOG_NOTICE,
		LOG_WARNING,
		LOG_ERROR,
		LOG_FATAL_ERROR,
		LOG_NONE
	};

	enum class LogTarget {
		CONSOLE,
		FILE,
		CONSOLE_AND_FILE
	};	

	//Used internally
	struct LoggerTargetInfo {
		LogTarget targetType;
		LogLevel level;

		string filename;
		ofFile *file;
	};

	//Used internally
	struct LogMessage {
		LogMessage (LogLevel level_, string module_) :
			level(level_),
			module(module_)
		{}

		stringstream* message;
		LogLevel level;
		string module;
		string timestamp;
	};

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

		stringstream& verbose (string module = "");
		stringstream& notice (string module = "");
		stringstream& warning (string module = "");
		stringstream& error (string module = "");
		stringstream& fatalError (string module = "");

		void level (LogLevel level, string module = "");
		void levelForConsole (LogLevel level);
		void levelForFile(LogLevel level, string filename = "CX_DEFERRED_LOGGER_DEFAULT");

		void flush (void); //BLOCKING

		void timestamps (bool logTimestamps, string format = "%H:%M:%S.%i");

		//ofEvent<LogEventData> messageFlushEvent;

		void setMessageFlushCallback (std::function<void(MessageFlushData&)> f);

	private:

		vector<LoggerTargetInfo> _targetInfo;
		map<string, LogLevel> _moduleLogLevels;
		vector<LogMessage> _messageQueue;

		stringstream& _log (LogLevel level, string module);
		string _getLogLevelName (LogLevel level);

		std::function<void(MessageFlushData&)> _flushCallback;

		bool _logTimestamps;
		string _timestampFormat;

	};

	namespace Instances {
		extern CX_Logger Log;
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