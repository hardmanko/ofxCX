#ifndef _CX_DEFERRED_LOGGER_H_
#define _CX_DEFERRED_LOGGER_H_

#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "ofUtils.h"
#include "ofFileUtils.h"
#include "ofEvents.h"

#include "CX_Clock.h"

//#undef ERROR //This is annoying and likely problematic.

using namespace std;

namespace CX {

	//Consider manually numbering these
	enum class LogLevel {
		LOG_ALL,
		VERBOSE,
		NOTICE,
		WARNING,
		LOG_ERROR,
		FATAL_ERROR,
		LOG_NONE
	};

	enum class LogTarget {
		CONSOLE,
		FILE,
		CONSOLE_AND_FILE
	};	

	struct LoggerTargetInfo {
		LogTarget targetType;
		LogLevel level;

		string filename;
		ofFile *file;
	};

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

	struct LogEventData {
		LogEventData (string message_, LogLevel level_, string module_) :
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

		bool timestamps;

		ofEvent<LogEventData> messageFlushEvent;

	private:

		vector<LoggerTargetInfo> _targetInfo;
		map<string, LogLevel> _moduleLogLevels;
		vector<LogMessage> _messageQueue;

		stringstream& _log (LogLevel level, string module);
		string _getLogLevelName (LogLevel level);

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