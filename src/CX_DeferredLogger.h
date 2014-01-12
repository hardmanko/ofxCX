#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

using namespace std;

namespace CX {

	//Consider manually numbering these
	enum class LogLevel {
		LOG_ALL,
		VERBOSE,
		NOTICE,
		WARNING,
		ERROR,
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
		//ofFile file;
	};

	struct LogMessage {

		LogMessage (LogLevel level_, string module_) :
			level(level_),
			module(module_)
		{}

		stringstream* message;
		LogLevel level;
		string module;
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

		//void to (LogTarget target, string filename = "");
		//void toConsole (bool yes);
		//void toFile (bool yes, string filename = "", LogLevel level = LogLevel::LOG_ALL);

		void flush (void); //BLOCKING

		bool timestamps;

		//ofEvent<LogEventData> messageFlushEvent;

	private:

		vector<LoggerTargetInfo> _targetInfo;
		map<string, LogLevel> _moduleLogLevels;
		vector<LogMessage> _messageQueue;

		stringstream& _log (LogLevel level, string module);
		string _getLogLevelName (LogLevel level);

	};

}

#include "stdint.h"

template <char series>
class kvrxSimpleADC {
public:
	int16_t read(uint8_t channel);
};

