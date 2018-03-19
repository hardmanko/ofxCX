#include "CX_Logger.h"

/*! This is an instance of CX::CX_Logger that is hooked into the CX backend.
All log messages generated by CX and openFrameworks go through this instance.
After runExperiment() returns, CX::Instances::Log.flush() is called.
\ingroup entryPoint
*/
CX::CX_Logger CX::Instances::Log;

namespace CX {
namespace Private {

	enum class LogTarget {
		CONSOLE,
		FILE,
		CONSOLE_AND_FILE
	};

	struct CX_LoggerTargetInfo {
		CX_LoggerTargetInfo(void) :
			file(nullptr)
		{}

		LogTarget targetType;
		CX_Logger::Level level;

		std::string filename;
		ofFile *file;
	};

	struct CX_LogMessage {
		CX_LogMessage(CX_Logger::Level level_, string module_) :
			level(level_),
			module(module_)
		{}

		std::string message;
		CX_Logger::Level level;
		std::string module;
		std::string timestamp;
	};

	struct CX_ofLogMessageEventData_t {
		ofLogLevel level;
		std::string module;
		std::string message;
	};

	//////////////////////
	// CX_LoggerChannel //
	//////////////////////

	class CX_LoggerChannel : public ofBaseLoggerChannel {
	public:
		~CX_LoggerChannel(void) {};

		void log(ofLogLevel level, const std::string & module, const std::string & message);
		void log(ofLogLevel level, const std::string & module, const char* format, ...);
		void log(ofLogLevel level, const std::string & module, const char* format, va_list args);

		ofEvent<CX_ofLogMessageEventData_t> messageLoggedEvent;
	};

	void CX_LoggerChannel::log(ofLogLevel level, const std::string & module, const std::string & message) {
		CX_ofLogMessageEventData_t md;
		md.level = level;
		md.module = module;
		md.message = message;
		ofNotifyEvent(this->messageLoggedEvent, md);
	}

	void CX_LoggerChannel::log(ofLogLevel level, const std::string & module, const char* format, ...) {
		va_list args;
		va_start(args, format);
		this->log(level, module, format, args);
		va_end(args);
	}

	void CX_LoggerChannel::log(ofLogLevel level, const std::string & module, const char* format, va_list args) {
		int bufferSize = 256;

		while (true) {

			char *buffer = new char[bufferSize];

			int result = vsnprintf(buffer, bufferSize - 1, format, args);
			if ((result > 0) && (result < bufferSize)) {
				this->log(level, module, std::string(buffer));

				delete[] buffer;
				buffer = nullptr;
				return;
			}

			delete[] buffer;
			buffer = nullptr;

			bufferSize *= 4;
			if (bufferSize > 17000) { //Largest possible is 16384 chars.
				this->log(ofLogLevel::OF_LOG_ERROR, "CX_LoggerChannel", "Could not convert formatted arguments: "
						  "Resulting message would have been too long.");
				return;
			}
		}
	}


	///////////////////////
	// CX_LogMessageSink //
	///////////////////////
	CX_LogMessageSink::CX_LogMessageSink(void) :
		_logger(nullptr)
	{
	}

	CX_LogMessageSink::CX_LogMessageSink(CX_LogMessageSink&& ms)
	{
	    std::swap(this->_message, ms._message);
	    std::swap(this->_logger, ms._logger);
	    std::swap(this->_level, ms._level);
	    std::swap(this->_module, ms._module);

        //Make sure that the swapped message has no logger.
		ms._logger = nullptr;
	}

	CX_LogMessageSink::CX_LogMessageSink(CX::CX_Logger* logger, CX::CX_Logger::Level level, std::string module) :
		_logger(logger),
		_level(level),
		_module(module)
	{
		_message = std::make_shared<std::ostringstream>();
	}

	/* This destructor is marked noexcept(false) because it sometimes throws exceptions
	(by design) in CX_Logger::_storeLogMessage() if exceptions are enabled with 
	CX_Logger::levelForExceptions().
	*/
	CX_LogMessageSink::~CX_LogMessageSink(void) noexcept(false) {
		if (_logger != nullptr) {
			_logger->_storeLogMessage(*this);
		}
	}

	CX_LogMessageSink& CX_LogMessageSink::operator << (std::ostream& (*func)(std::ostream&)) {
		func(*_message);
		return *this;
	}

} //namespace Private



CX_Logger::CX_Logger(void) :
	_logTimestamps(false),
	_timestampFormat("%H:%M:%S"),
	_defaultLogLevel(Level::LOG_NOTICE)
{
	levelForAllExceptions(Level::LOG_NONE);
	levelForConsole(Level::LOG_ALL);

	_ofLoggerChannel = std::make_shared<CX::Private::CX_LoggerChannel>();

	ofAddListener(_ofLoggerChannel->messageLoggedEvent, this, &CX_Logger::_loggerChannelEventHandler);

	levelForAllModules(Level::LOG_ERROR);
}

CX_Logger::~CX_Logger(void) {
	this->captureOFLogMessages(false);

	//Doesn't need to be removed because the _ofLoggerChannel is being destructed along with its messageLoggedEvent.
	//ofRemoveListener(_ofLoggerChannel->messageLoggedEvent, this, &CX_Logger::_loggerChannelEventHandler);

	flush();

	for (unsigned int i = 0; i < _targetInfo.size(); i++) {
		if (_targetInfo[i].targetType == CX::Private::LogTarget::FILE) {
			//_targetInfo[i].file->close(); //They should already be closed from flush()
			delete _targetInfo[i].file;
		}
	}
}

/*! Log all of the messages stored since the last call to flush() to the
selected logging targets. This is a blocking operation, because it may take
quite a while to output all log messages to various targets (see \ref blockingCode).
\note This function is not 100% thread-safe: Only call it from the main thread. */
void CX_Logger::flush(void) {

	//By getting the message count once and only iterating over that many messages,
	//a known number of messages are processed and just that many messages can be deleted later.
	_messageQueueMutex.lock();
	unsigned int messageCount = _messageQueue.size();
	_messageQueueMutex.unlock();

	if (messageCount == 0) {
		return;
	}

	//Open output files
	for (unsigned int i = 0; i < _targetInfo.size(); i++) {
		if (_targetInfo[i].targetType == CX::Private::LogTarget::FILE) {
			_targetInfo[i].file->open(_targetInfo[i].filename, ofFile::Append, false);
			if (!_targetInfo[i].file->is_open()) {
				std::cerr << "<CX_Logger> File " << _targetInfo[i].filename << " could not be opened for logging." << std::endl;
			}
		}
	}

	for (unsigned int i = 0; i < messageCount; i++) {
		//Lock and copy each message. Messages cannot be added while locked.
		//After the unlock, the message copy is used, not the original message.
		_messageQueueMutex.lock();
		CX::Private::CX_LogMessage m = _messageQueue[i];
		_messageQueueMutex.unlock();

		if (flushEvent.size() > 0) {
			MessageFlushData dat(m.message, m.level, m.module);
			ofNotifyEvent(flushEvent, dat);
		}

		std::string formattedMessage = _formatMessage(m) + "\n";

		if (m.level >= _moduleLogLevels[m.module]) {
			for (unsigned int i = 0; i < _targetInfo.size(); i++) {
				if (m.level >= _targetInfo[i].level) {
					if (_targetInfo[i].targetType == CX::Private::LogTarget::CONSOLE) {
						std::cout << formattedMessage;
					} else if (_targetInfo[i].targetType == CX::Private::LogTarget::FILE) {
						*_targetInfo[i].file << formattedMessage;
					}
				}
			}
		}
	}

	//Close output files
	for (unsigned int i = 0; i < _targetInfo.size(); i++) {
		if (_targetInfo[i].targetType == CX::Private::LogTarget::FILE) {
			_targetInfo[i].file->close();
		}
	}

	//Delete printed messages
	_messageQueueMutex.lock();
	_messageQueue.erase(_messageQueue.begin(), _messageQueue.begin() + messageCount);
	_messageQueueMutex.unlock();
}

/*! \brief Clear all stored log messages. */
void CX_Logger::clear(void) {
	_messageQueueMutex.lock();
	_messageQueue.clear();
	_messageQueueMutex.unlock();
}

/*! \brief Set the log level for messages to be printed to the console. */
void CX_Logger::levelForConsole(Level level) {
	bool consoleFound = false;
	for (unsigned int i = 0; i < _targetInfo.size(); i++) {
		if (_targetInfo[i].targetType == CX::Private::LogTarget::CONSOLE) {
			consoleFound = true;
			_targetInfo[i].level = level;
		}
	}

	if (!consoleFound) {
		CX::Private::CX_LoggerTargetInfo consoleTarget;
		consoleTarget.targetType = CX::Private::LogTarget::CONSOLE;
		consoleTarget.level = level;
		_targetInfo.push_back(consoleTarget);
	}
}

/*! Sets the log level for the file with the given file name. If the file does not exist, it will be created.
If the file does exist, it will be overwritten with a warning logged to cerr (typically the console).
\param level Log messages with level greater than or equal to this level will be outputted to the file.
See the \ref CX::CX_Logger::Level enum for valid values.
\param filename The name of the file to output to. If no file name is given, a file with name
generated from a date/time from the start time of the experiment will be used.
*/
void CX_Logger::levelForFile(Level level, std::string filename) {
	if (filename == "CX_LOGGER_DEFAULT") {
		filename = "Log file " + CX::Instances::Clock.getExperimentStartDateTimeString("%Y-%b-%e %h-%M-%S %a") + ".txt";
	}
	filename = ofToDataPath(filename);

	bool fileFound = false;
	unsigned int fileIndex = -1;
	for (unsigned int i = 0; i < _targetInfo.size(); i++) {
		if ((_targetInfo[i].targetType == CX::Private::LogTarget::FILE) && (_targetInfo[i].filename == filename)) {
			fileFound = true;
			fileIndex = i;
			_targetInfo[i].level = level;
		}
	}

	//If nothing is to be logged, either delete or never create the target
	if (level == Level::LOG_NONE) {
		if (fileFound) {
			_targetInfo.erase(_targetInfo.begin() + fileIndex);
		}
		return;
	}

	if (!fileFound) {
		CX::Private::CX_LoggerTargetInfo fileTarget;
		fileTarget.targetType = CX::Private::LogTarget::FILE;
		fileTarget.level = level;
		fileTarget.filename = filename;
		fileTarget.file = new ofFile(); //This is deallocated in the dtor

		fileTarget.file->open(filename, ofFile::Reference, false);
		if (fileTarget.file->exists()) {
			std::cerr << "<CX_Logger> Log file already exists with name: " << filename << ". It will be overwritten." << std::endl;
		}

		fileTarget.file->open(filename, ofFile::WriteOnly, false);
		if (fileTarget.file->is_open()) {
			std::cout << "<CX_Logger> Log file \"" + filename + "\" opened." << std::endl;
		}
		*fileTarget.file << "CX log file. Created " << CX::Instances::Clock.getDateTimeString() << std::endl;
		fileTarget.file->close();

		_targetInfo.push_back(fileTarget);
	}
}

/*! Sets the log level for the given module. Messages from that module that are at a lower level than
\ref level will be ignored.
\param level See the \ref CX::CX_Logger::Level enum for valid values.
\param module A string representing one of the modules from which log messages are generated.
*/
void CX_Logger::levelForModule(Level level, std::string module) {
	_moduleLogLevelsMutex.lock();
	_moduleLogLevels[module] = level;
	_moduleLogLevelsMutex.unlock();
}

/*! Gets the log level in use by the given module.
\param module The name of the module.
\return The level for `module`. */
CX_Logger::Level CX_Logger::getModuleLevel(std::string module) {
	Level level = _defaultLogLevel;
	_moduleLogLevelsMutex.lock();
	if (_moduleLogLevels.find(module) != _moduleLogLevels.end()) {
		level = _moduleLogLevels[module];
	}
	_moduleLogLevelsMutex.unlock();
	return level;
}

/*!
Set the log level for all modules. This works both retroactively and proactively: All currently known modules
are given the log level and the default log level for new modules as set to the level.
*/
void CX_Logger::levelForAllModules(Level level) {
	_moduleLogLevelsMutex.lock();
	_defaultLogLevel = level;
	for (std::map<std::string, Level>::iterator it = _moduleLogLevels.begin(); it != _moduleLogLevels.end(); it++) {
		_moduleLogLevels[it->first] = level;
	}
	_moduleLogLevelsMutex.unlock();
}


/*! Set whether or not to log timestamps and the format for the timestamps.
\param logTimestamps Does what it says.
\param format Timestamp format string. See http://pocoproject.org/docs/Poco.DateTimeFormatter.html#4684 for
documentation of the format. Defaults to %H:%M:%S.%i (24-hour clock with milliseconds at the end).
*/
void CX_Logger::timestamps(bool logTimestamps, std::string format) {
	_logTimestamps = logTimestamps;
	_timestampFormat = format;
}

/*! This is the fundamental logging function for this class. Example use:
\code{.cpp}
Log.log(CX_Logger::Level::LOG_WARNING, "moduleName") << "Special message number: " << 20;
\endcode

Possible output: "[warning] <moduleName> Special message number: 20"

A newline is inserted automatically at the end of each message.

\param level Log level for this message. This has implications for message filtering. See CX::CX_Logger::level().
This should not be LOG_ALL or LOG_NONE, because that would be weird, wouldn't it?
\param module Name of the module that this log message is related to. This has implications for message filtering.
See CX::CX_Logger::level().
\return An object that can have log messages given to it as though it were a std::ostream.
\note This function and all of the trivial wrappers of this function (verbose(), notice(), warning(),
error(), fatalError()) are thread-safe.
*/
CX::Private::CX_LogMessageSink CX_Logger::log(Level level, std::string module) {
	return _log(level, module);
}

/*! \brief Equivalent to `log(CX_Logger::Level::LOG_VERBOSE, module)`. */
CX::Private::CX_LogMessageSink CX_Logger::verbose(std::string module) {
	return _log(Level::LOG_VERBOSE, module);
}

/*! \brief Equivalent to `log(CX_Logger::Level::LOG_NOTICE, module)`. */
CX::Private::CX_LogMessageSink CX_Logger::notice(std::string module) {
	return _log(Level::LOG_NOTICE, module);
}

/*! \brief Equivalent to `log(CX_Logger::Level::LOG_WARNING, module)`. */
CX::Private::CX_LogMessageSink CX_Logger::warning(std::string module) {
	return _log(Level::LOG_WARNING, module);
}

/*! \brief Equivalent to `log(CX_Logger::Level::LOG_ERROR, module)`. */
CX::Private::CX_LogMessageSink CX_Logger::error(std::string module) {
	return _log(Level::LOG_ERROR, module);
}

/*! \brief Equivalent to `log(CX_Logger::Level::LOG_FATAL_ERROR, module)`. */
CX::Private::CX_LogMessageSink CX_Logger::fatalError(std::string module) {
	return _log(Level::LOG_FATAL_ERROR, module);
}

/*! \brief Equivalent to `log(CX_Logger::Level::LOG_NOTICE, module)`. */
CX::Private::CX_LogMessageSink CX_Logger::operator()(std::string module) {
	return _log(Level::LOG_NOTICE, module);
}

/*! Set this instance of CX_Logger to be the target of any messages created by openFrameworks logging functions.

This function is called during CX setup for CX::Instances::Log, so you do not need to call it yourself for that instance. 

\param capture If `true`, capture oF log messages. If `false`, oF log messages go to the console.
*/
void CX_Logger::captureOFLogMessages(bool capture) {
	if (capture) {
		ofSetLoggerChannel(ofPtr<ofBaseLoggerChannel>(this->_ofLoggerChannel));
		ofSetLogLevel(ofLogLevel::OF_LOG_VERBOSE);
	} else {
		ofLogToConsole();
	}
}

/*! See CX::CX_Logger::levelForExceptions() for more information.

\param level The default exception level.
*/
void CX_Logger::levelForAllExceptions(Level level) {
	_exceptionLevelsMutex.lock();
	_defaultExceptionLevel = level;
	for (std::map<std::string, Level>::iterator it = _exceptionLevels.begin(); it != _exceptionLevels.end(); it++) {
		_exceptionLevels[it->first] = level;
	}
	_exceptionLevelsMutex.unlock();
}

/*! When a logged message is stored, if its log level is greater than or
equal to the exception level for the given module, an exception will be thrown.
The exception will be a std::runtime_error. By default, the exception level is LOG_NONE,
i.e. that no logged messages will cause an exception to be thrown.

You might want to use this feature for two reasons:
1) There are certain really serious errors that sometimes happen while
the experiment is running that are not themselves exceptions but that
you want to be exceptions so that they will not allow the program to
continue in an erroneous state.
2) For debugging purposes. When an exception is thrown it triggers a
breakpoint in some IDEs. When that happens, you have a full stack trace
and interactive debugger environment to work with to help determine why
the logged message was logged.

Note that, for technical reasons, this exception throwing feature
may not work properly on some systems. If you get unexpected program
termination when trying to use this feature, you may just want to not
use it.

\param level The desired exception level. The naming of the values in CX_LogLevel
is slightly confusing in the context of this function. Instead of, e.g., LOG_WARNING,
think of the value as EXCEPTION_ON_WARNING (or greater).
\param module The module to set the exception level for.
*/
void CX_Logger::levelForExceptions(Level level, std::string module) {
	_exceptionLevelsMutex.lock();
	_exceptionLevels[module] = level;
	_exceptionLevelsMutex.unlock();
}

void CX_Logger::_storeLogMessage(CX::Private::CX_LogMessageSink& ms) {
	//If the module is unknown to the logger, it becomes known with the default log level.
	_moduleLogLevelsMutex.lock();
	if (_moduleLogLevels.find(ms._module) == _moduleLogLevels.end()) {
		_moduleLogLevels[ms._module] = _defaultLogLevel;
	}
	_moduleLogLevelsMutex.unlock();


	CX::Private::CX_LogMessage temp(ms._level, ms._module);
	temp.message = (*(ms._message)).str();

	if (_logTimestamps) {
		temp.timestamp = CX::Instances::Clock.getDateTimeString(_timestampFormat);
	}

	_messageQueueMutex.lock();
	_messageQueue.push_back(temp);
	_messageQueueMutex.unlock();

	//Check for exceptions
	_exceptionLevelsMutex.lock();
	Level level;
	if (_exceptionLevels.find(ms._module) != _exceptionLevels.end()) {
		level = _exceptionLevels[ms._module];
	} else {
		level = _defaultExceptionLevel;
	}
	_exceptionLevelsMutex.unlock();

	if (ms._level >= level && !std::uncaught_exception()) {
		std::string formattedMessage = _formatMessage(temp);
		throw std::runtime_error(formattedMessage);
	}
}

CX::Private::CX_LogMessageSink CX_Logger::_log(Level level, std::string module) {
	return CX::Private::CX_LogMessageSink(this, level, module);
}

std::string CX_Logger::_getLogLevelString(Level level) {
	switch (level) {
	case Level::LOG_VERBOSE: return "verbose";
	case Level::LOG_NOTICE: return "notice";
	case Level::LOG_WARNING: return "warning";
	case Level::LOG_ERROR: return "error";
	case Level::LOG_FATAL_ERROR: return "fatal";
	case Level::LOG_ALL: return "all";
	case Level::LOG_NONE: return "none";
	};
	return "";
}

std::string CX_Logger::_formatMessage(const CX::Private::CX_LogMessage& message) {

	std::string formattedMessage;
	if (_logTimestamps) {
		formattedMessage += message.timestamp + " ";
	}

	std::string logName = _getLogLevelString(message.level);
	logName.append(max<int>((int)(7 - logName.size()), 0), ' '); //Pad out names to 7 chars
	formattedMessage += "[ " + logName + " ] ";

	if (message.module != "") {
		formattedMessage += "<" + message.module + "> ";
	}

	formattedMessage += message.message;

	return formattedMessage;
}

void CX_Logger::_loggerChannelEventHandler(CX::Private::CX_ofLogMessageEventData_t& md) {
	Level convertedLevel = Level::LOG_NOTICE;

	switch (md.level) {
	case ofLogLevel::OF_LOG_VERBOSE: convertedLevel = Level::LOG_VERBOSE; break;
	case ofLogLevel::OF_LOG_NOTICE: convertedLevel = Level::LOG_NOTICE; break;
	case ofLogLevel::OF_LOG_WARNING: convertedLevel = Level::LOG_WARNING; break;
	case ofLogLevel::OF_LOG_ERROR: convertedLevel = Level::LOG_ERROR; break;
	case ofLogLevel::OF_LOG_FATAL_ERROR: convertedLevel = Level::LOG_FATAL_ERROR; break;
	case ofLogLevel::OF_LOG_SILENT: convertedLevel = Level::LOG_NONE; break;
	}

	this->_log(convertedLevel, md.module) << md.message;
}

} // namespace CX
