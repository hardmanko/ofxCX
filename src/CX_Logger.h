#pragma once

#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <mutex>

#include <stdio.h> //vsnprintf
#include <stdarg.h> //va_args

#include "ofUtils.h"
#include "ofFileUtils.h"
#include "ofEvents.h"
#include "ofLog.h"
#include "ofTypes.h"

#include "CX_Time_t.h"

/*! \defgroup errorLogging Message Logging
This module is designed for logging error, warnings, and other messages. 
The primary interface is the \ref CX_Logger class,
in particular the preinstantiated \ref CX::Instances::Log.

Basic logging looks like:
\code {.cpp}
// Create logfile that takes all logged messages using a filename created 
// with a date/time string and placed in the Logfiles subdirectory
Log.levelForFile(CX_Logger::Level::LOG_ALL);

// Log messages anywhere, any time in your code
Log.notice() << "An information notice.";

// Flush messages to logging targets (console and optional files).
Log.flush();
// Printing the logged messages takes time, so only flush() 
// in non-timing-critical sections of code.

\endcode

The messageLogging example contains examples of how to:
+ Add logging/debugging messages to your code.
+ Print logging/debugging messages to the console and files.
+ Filter which messages get printed by the logging level of the message (notice, warning, error, etc.).
+ And more.
See examples/messageLogging/src/messageLogging.cpp
*/

#define CX_LOGGER_FILE_LINE_STR "File: " << __FILE__ << ", Line: " << __LINE__

namespace CX {

	//Forward declarations of internally used structs and classes
	struct CX_LogMessage;

	namespace Private {
		struct CX_LoggerTargetInfo;
		class CX_LoggerChannel;
		struct CX_ofLogMessageEventData_t;
		class CX_LogMessageSink;
	}

	/*! This class is used for logging messages throughout the CX backend code. 
	It is also recommended that users of CX use this class for logging in their experiment code.
	
	Rather than instantiating your own CX_Logger, it is better to use \ref CX::Instances::Log
	which has been set up and is ready to use in user code as in this minimal working example:

	\code{.c++}
	#include "CX.h"

	void runExperiment(void) {
		// Log a message.
		Log.warning() << "This is a warning.";

		// Flush the message to logging targets (check the console).
		Log.flush();

		// Wait for user to exit.
		Input.Keyboard.waitForKeypress(-1);
	}
	\endcode

	Additional code examples can be found in the "logging" example that demonstrates many of the 
	features of CX_Logger.

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
	of which only contains errors and fatal errors. 
	
	By default, no logfiles are created and messages are printed to the console. Verbose messages
	are not printed to the console by default.

	Logfiles are created using the `levelForFile()` function
	\code{.cpp}
	Log.levelForFile(CX_Logger::Level::LOG_ALL);

	Log.levelForFile(CX_Logger::Level::LOG_WARNINGS);
	\endcode

	There are a few openFrameworks classes that are extremely verbose, like ofFbo, and less severe 
	messages from those classes	are suppressed by default. 
	To get all messages from all modules, call CX_Logger::levelForAllModules(CX_Logger::Level::LOG_ALL).

	This class is designed to be partially thread safe. It is safe to use any of the message logging
	functions (log(), verbose(), notice(), warning(), error(), and fatalError()) in multiple threads
	at once. Other than those functions, functions should be called only from the main thread.

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


		CX_Logger(void);
		~CX_Logger(void);


		// Setup
		void levelForFile(Level level, std::string filename = "CX_LOGGER_DEFAULT", std::string subdir = "Logfiles");
		void levelForConsole(Level level);
		
		void levelForModule(Level level, std::string module);
		void levelForAllModules(Level level);
		Level getModuleLevel(std::string module); // TODO: Remove?

		void levelForExceptions(Level level, std::string module);
		void levelForAllExceptions(Level level);

		void timestamps(bool logTimestamps, std::string format = "%H:%M:%S.%m");

		void captureOFLogMessages(bool capture);


		// Logging functions
		CX::Private::CX_LogMessageSink log(Level level, std::string module = "");
		CX::Private::CX_LogMessageSink operator()(std::string module = "");

		CX::Private::CX_LogMessageSink verbose(std::string module = "");
		CX::Private::CX_LogMessageSink notice(std::string module = "");
		CX::Private::CX_LogMessageSink warning(std::string module = "");
		CX::Private::CX_LogMessageSink error(std::string module = "");
		CX::Private::CX_LogMessageSink fatalError(std::string module = "");


		// Flushing messages
		void flush(void);
		//void clear(void); // TODO: Finish removing?

		/*! When `flush()` is called, listeners to `flushEvent` will be passed a `CX_LogMessage` struct
		for each message in the queue. No filtering is performed: All messages regardless of the module
		log level will be sent to listeners. */
		ofEvent<const CX_LogMessage&> flushEvent;


		CX_LogMessage getLastMessage(void); // TODO: Remove?

		

	private:

		CX::Private::CX_LogMessageSink _log(Level level, std::string module);

		friend class CX::Private::CX_LogMessageSink;
		void _storeLogMessage(CX::Private::CX_LogMessageSink& msg);

		std::shared_ptr<CX::Private::CX_LoggerChannel> _ofLoggerChannel;
		void _loggerChannelEventHandler(CX::Private::CX_ofLogMessageEventData_t& md);

		static std::string _getLogLevelString(Level level);
		std::string _formatMessage(const CX_LogMessage& message, std::string timestampFormat);

		std::string _getTimestampFormat(void);

		// Data
		std::vector<CX::Private::CX_LoggerTargetInfo> _targetInfo; // only used in flush, levelForConsole, and levelForFile

		std::recursive_mutex _timestampMutex;
		std::string _timestampFormat;

		std::recursive_mutex _messageQueueMutex;
		std::vector<CX_LogMessage> _messageQueue;
		
		std::recursive_mutex _moduleLogLevelsMutex;
		std::map<std::string, Level> _moduleLogLevels;
		Level _defaultModuleLevel;

		std::recursive_mutex _exceptionLevelsMutex;
		std::map<std::string, Level> _exceptionLevels;
		Level _defaultExceptionLevel;


	};

	namespace Instances {
		extern CX_Logger Log;
	}



	/*! Contains information from a single logged message. */
	struct CX_LogMessage {
		CX_LogMessage(void) :
			level(CX_Logger::Level::LOG_NONE)
		{}

		CX_LogMessage(CX_Logger::Level level_, std::string module_) :
			level(level_),
			module(module_)
		{}

		std::string message; //!< A string containing the logged message.
		CX_Logger::Level level; //!< The log level of the message.
		std::string module; //!< The module associated with the message, usually which created the message.
		CX_Millis timestamp; //!< The time at which the message was logged.
	};



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
