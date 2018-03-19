#pragma once

#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>

#include <stdio.h> //vsnprintf
#include <stdarg.h> //va_args

#include "Poco/Mutex.h"

#include "ofUtils.h"
#include "ofFileUtils.h"
#include "ofEvents.h"
#include "ofLog.h"

#include "CX_Clock.h"

/*! \defgroup errorLogging Message Logging
This module is designed for logging error, warnings, and other messages. The primary interface is the CX_Logger class,
in particular the preinstantiated CX::Instances::Log.
*/

namespace CX {

	namespace Private {
		//Forward declarations of internally used structs and classes
		struct CX_LogMessage;
		struct CX_LoggerTargetInfo;
		class CX_LoggerChannel;
		struct CX_ofLogMessageEventData_t;
		class CX_LogMessageSink;
	}

	/*! This class is used for logging messages throughout the CX backend code. It can also be used
	in user code to log messages. Rather than instantiating your own copy of CX_Logger, it is much
	better to use the preinstantiated \ref CX::Instances::Log.

	example-logging shows a number of the features of CX_Logger.

	CX_Logger is designed to help prevent timing errors. Messages can be logged at any time during
	program execution. If these messages were immediately outputted to the console or to files,
	it could disrupt a timing-critical section of code. For this reason, logged messages are stored
	by the CX_Logger until the user requests that all stored messages be outputted to the logging
	targets with CX_Logger::flush(). The user can choose an appropriate, non-timing-critical time
	at which to call flush().

	By default, messages are logged to the console window that opens with CX programs. Optionally,
	messages can also be logged to any number of files using CX_Logger::levelForFile(). For each
	logging target (i.e. the console and the logfiles), you can filter out less severe messages.
	For example, you could have two log files, one of which contains all messages and the other
	of which only contains errors and fatal errors. By default, no logfiles are created and
	all messages (with only a few exceptions) are logged to the console. There are a few openFrameworks
	classes that are extremely verbose, like ofFbo, and less severe messages from those classes
	are suppressed by default. You can undo this behavior by simply calling CX_Logger::levelForAllModules()
	with CX_Logger::Level::LOG_ALL as the argument.

	This class is designed to be partially thread safe. It is safe to use any of the message logging
	functions (log(), verbose(), notice(), warning(), error(), and fatalError()) in multiple threads
	at once. Other than those functions, the other functions should be called only from one thread
	(the main thread).

	\ingroup errorLogging */
	class CX_Logger {
	public:

		/*! \enum Level
		Log levels for log messages. Depending on the log level chosen, the name of the level will be printed before the message.
		Depending on the settings set using level(), levelForConsole(), or levelForFile(), if the log level of a message is below
		the level set for the module or logging target it will not be printed. For example, if LOG_ERROR is the level for the console
		and LOG_NOTICE is the level for the module "test", then messages logged to the "test" module will be completely ignored if
		at verbose level (because of the module setting) and will not be printed to the console if they are below the level of an
		error (because of the console setting).
		\ingroup errorLogging
		*/
		enum class Level : int {
			//These rely on numeric values being ordered: Do not change the values.
			LOG_ALL = 0, //This is functionally identical to LOG_VERBOSE, but it is more clear about what it does.
			LOG_VERBOSE = 1,
			LOG_NOTICE = 2,
			LOG_WARNING = 3,
			LOG_ERROR = 4,
			LOG_FATAL_ERROR = 5,
			LOG_NONE = 6
		};

		/*! If a user function is listening for flush callbacks by using setMessageFlushCallback(), each time the user
		function is called, it gets a reference to an instance of this struct with all the information filled in. */
		struct MessageFlushData {
			MessageFlushData(void) :
				message(""),
				level(CX_Logger::Level::LOG_FATAL_ERROR),
				module("")
			{}

			/*! Convenience constructor which constructs an instance of the struct with the provided values. */
			MessageFlushData(std::string message_, CX_Logger::Level level_, std::string module_) :
				message(message_),
				level(level_),
				module(module_)
			{}

			std::string message; //!< A string containing the logged message.
			Level level; //!< The log level of the message.
			std::string module; //!< The module associated with the message, usually which created the message.
		};


		CX_Logger(void);
		~CX_Logger(void);

		CX::Private::CX_LogMessageSink log(Level level, std::string module = "");
		CX::Private::CX_LogMessageSink verbose(std::string module = "");
		CX::Private::CX_LogMessageSink notice(std::string module = "");
		CX::Private::CX_LogMessageSink warning(std::string module = "");
		CX::Private::CX_LogMessageSink error(std::string module = "");
		CX::Private::CX_LogMessageSink fatalError(std::string module = "");
		CX::Private::CX_LogMessageSink operator()(std::string module = "");

		void flush(void);
		void clear(void);

		void levelForModule(Level level, std::string module);
		void levelForAllModules(Level level);

		void levelForConsole (Level level);
		void levelForFile(Level level, std::string filename = "CX_LOGGER_DEFAULT");

		void levelForAllExceptions(Level level);
		void levelForExceptions(Level level, std::string module);

		Level getModuleLevel(std::string module);

		void timestamps(bool logTimestamps, std::string format = "%H:%M:%S.%i");

		/*! When flush() is called, listeners to `flushEvent` will be passed a `MessageFlushData` struct
		for each message in the queue. No filtering is performed: All messages regardless of the module
		log level will be sent to listeners. */
		ofEvent<const MessageFlushData&> flushEvent;

		void captureOFLogMessages(bool capture);

	private:

		std::vector<CX::Private::CX_LoggerTargetInfo> _targetInfo;

		Poco::Mutex _messageQueueMutex;
		std::vector<CX::Private::CX_LogMessage> _messageQueue;
		
		Poco::Mutex _moduleLogLevelsMutex;
		std::map<std::string, Level> _moduleLogLevels;


		CX::Private::CX_LogMessageSink _log(Level level, std::string module);

		friend class CX::Private::CX_LogMessageSink;
		void _storeLogMessage(CX::Private::CX_LogMessageSink& msg);

		bool _logTimestamps;
		std::string _timestampFormat;

		Level _defaultLogLevel;

		Poco::Mutex _exceptionLevelsMutex;
		std::map<std::string, Level> _exceptionLevels;
		Level _defaultExceptionLevel;

		std::shared_ptr<CX::Private::CX_LoggerChannel> _ofLoggerChannel;
		void _loggerChannelEventHandler(CX::Private::CX_ofLogMessageEventData_t& md);

		static std::string _getLogLevelString(Level level);
		std::string _formatMessage(const CX::Private::CX_LogMessage& message);
	};

	namespace Instances {
		extern CX_Logger Log;
	}

	namespace Private {
		//This class is based very directly on ofLog, except that it has a private constructor
		//so that it can only be made by a CX_Logger.
		class CX_LogMessageSink {
		public:
			~CX_LogMessageSink(void) noexcept(false);

			CX_LogMessageSink& operator<<(std::ostream& (*func)(std::ostream&));

			template <class T>
			CX_LogMessageSink& operator<<(const T& value) {
				*_message << value;
				return *this;
			}

		private:
			friend class CX::CX_Logger;

			CX_LogMessageSink(void);
			CX_LogMessageSink(CX_LogMessageSink&& ms);
			CX_LogMessageSink(CX::CX_Logger* logger, CX::CX_Logger::Level level, std::string module);

			CX_Logger* _logger;

			std::shared_ptr<std::ostringstream> _message;
			CX::CX_Logger::Level _level;
			std::string _module;
		};
	}
}
