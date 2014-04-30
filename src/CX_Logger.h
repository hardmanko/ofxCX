#pragma once

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

/*! \defgroup errorLogging Error Logging

*/

namespace CX {

	/*! \enum CX_LogLevel
	Log levels for log messages. Depending on the log level chosen, the name of the level will be printed before the message.
	Depending on the settings set using level(), levelForConsole(), or levelForFile(), if the log level of a message is below
	the level set for the module or logging target it will not be printed. For example, if LOG_ERROR is the level for the console
	and LOG_NOTICE is the level for the module "test", then messages logged to the "test" module will be completely ignored if
	at verbose level (because of the module setting) and will not be printed to the console if they are below the level of an
	error (because of the console setting).
	\ingroup errorLogging
	*/
	//These rely on ordering: do not change it.
	enum class CX_LogLevel {
		LOG_ALL, //This is functionally identical to LOG_VERBOSE, but it is more explicit about what it does.
		LOG_VERBOSE,
		LOG_NOTICE,
		LOG_WARNING,
		LOG_ERROR,
		LOG_FATAL_ERROR,
		LOG_NONE
	};

	/*! If a user function is listening for flush callbacks by using setMessageFlushCallback(), the user
	function gets a reference to an instance of this struct containing the given information. */
	struct CX_MessageFlushData {
		CX_MessageFlushData(void) :
			message(""),
			level(CX_LogLevel::LOG_FATAL_ERROR),
			module("")
		{}
		CX_MessageFlushData(std::string message_, CX_LogLevel level_, std::string module_) :
			message(message_),
			level(level_),
			module(module_)
		{}
		std::string message; //!< A string containing the logged message.
		CX_LogLevel level; //!< The log level of the message.
		std::string module; //!< The module associated with the message.
	};

	//Forward declarations of internally used structs
	struct CX_LogMessage;
	struct CX_LoggerTargetInfo;

	/*! This class is used for logging messages throughout the CX backend code. It can also be used
	in user code to log messages. Rather than instantiating your own copy of CX_Logger, it is probably
	better to use the preinstantiated \ref CX::Instances::Log.

	There is an example showing a number of the features of CX_Logger named example-logging.
	\ingroup errorLogging */
	class CX_Logger {
	public:

		CX_Logger (void);
		~CX_Logger (void);

		std::stringstream& log(CX_LogLevel level, std::string module = "");
		std::stringstream& verbose(std::string module = "");
		std::stringstream& notice(std::string module = "");
		std::stringstream& warning(std::string module = "");
		std::stringstream& error(std::string module = "");
		std::stringstream& fatalError(std::string module = "");

		void level(CX_LogLevel level, std::string module = "");
		void levelForConsole (CX_LogLevel level);
		void levelForFile(CX_LogLevel level, std::string filename = "CX_DEFERRED_LOGGER_DEFAULT");
		void levelForAllModules(CX_LogLevel level);

		CX_LogLevel getModuleLevel(std::string module);

		void flush (void);

		void timestamps(bool logTimestamps, std::string format = "%H:%M:%S.%i");

		void setMessageFlushCallback(std::function<void(CX_MessageFlushData&)> f);

		void captureOFLogMessages(void);

	private:
		std::vector<CX_LoggerTargetInfo> _targetInfo;
		std::map<std::string, CX_LogLevel> _moduleLogLevels;
		std::vector<CX_LogMessage> _messageQueue;
		Poco::Mutex _messageQueueMutex;
		Poco::Mutex _moduleLogLevelsMutex;

		std::stringstream& _log(CX_LogLevel level, std::string module);
		std::string _getLogLevelName(CX_LogLevel level);

		std::function<void(CX_MessageFlushData&)> _flushCallback;

		bool _logTimestamps;
		std::string _timestampFormat;

		CX_LogLevel _defaultLogLevel;

		ofPtr<CX_LoggerChannel> _ofLoggerChannel;
		void _loggerChannelEventHandler(CX_ofLogMessageEventData_t& md);
	};

	namespace Instances {
		extern CX_Logger Log;
	}
}