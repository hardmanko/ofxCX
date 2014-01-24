#include "CX_Logger.h"

using namespace CX;

CX_Logger CX::Instances::Log;

enum class LogTarget {
	CONSOLE,
	FILE,
	CONSOLE_AND_FILE
};

struct CX::LoggerTargetInfo {
	LoggerTargetInfo (void) :
		file(nullptr)
	{}

	LogTarget targetType;
	LogLevel level;

	string filename;
	ofFile *file;
};


struct CX::LogMessage {
	LogMessage (LogLevel level_, string module_) :
		level(level_),
		module(module_)
	{}

	stringstream* message;
	LogLevel level;
	string module;
	string timestamp;
};


CX_Logger::CX_Logger (void) :
	_logTimestamps(false),
	_flushCallback(nullptr),
	_timestampFormat("%H:%M:%S"),
	_defaultLogLevel(LogLevel::LOG_NOTICE)
{
	levelForConsole(LogLevel::LOG_ALL);
}

CX_Logger::~CX_Logger (void) {
	flush();
	for (unsigned int i = 0; i < _targetInfo.size(); i++) {
		if (_targetInfo[i].targetType == LogTarget::FILE) {
			_targetInfo[i].file->close();
			delete _targetInfo[i].file;
		}
	}
}

//This function is called at the start of a new messsage.
stringstream& CX_Logger::_log (LogLevel level, string module) {

	if (_moduleLogLevels.find(module) == _moduleLogLevels.end()) {
		_moduleLogLevels[module] = _defaultLogLevel;
	}
	
	_messageQueue.push_back( LogMessage(level, module) );
	_messageQueue.back().message = new stringstream; //Manually allocated: Must deallocate later.

	if (_logTimestamps) {
		_messageQueue.back().timestamp = CX::Instances::Clock.getDateTimeString(_timestampFormat);
	}

	return *(_messageQueue.back().message);
}

/*! Log all of the messages stored since the last call to flush() to the selected logging targets. This is a BLOCKING operation. */
void CX_Logger::flush (void) {

	for (unsigned int i = 0; i < _targetInfo.size(); i++) {
		if (_targetInfo[i].targetType == LogTarget::FILE) {
			_targetInfo[i].file->open(_targetInfo[i].filename, ofFile::Append, false);
			if (!_targetInfo[i].file->is_open()) {
				cerr << "File " << _targetInfo[i].filename << " not opened for logging." << endl;
			}
		}
	}

	for (auto it = _messageQueue.begin(); it != _messageQueue.end(); it++) {
		MessageFlushData dat( it->message->str(), it->level, it->module );
		//ofNotifyEvent(this->messageFlushEvent, dat );
		if (_flushCallback) {
			_flushCallback( dat );
		}

		string logName = _getLogLevelName(it->level);
		logName.append( max<int>((int)(7 - logName.size()), 0), ' ' ); //Pad out names to 7 chars
		string formattedMessage;
		if (_logTimestamps) {
			formattedMessage += it->timestamp + " ";
		}

		formattedMessage += "[ " + logName + " ] ";

		if (it->module != "") {
			formattedMessage += "<" + it->module + "> ";
		}

		*it->message << endl;

		formattedMessage += it->message->str();

		if (it->level >= _moduleLogLevels[it->module]) {
			for (unsigned int i = 0; i < _targetInfo.size(); i++) {
				if (it->level >= _targetInfo[i].level) {
					if (_targetInfo[i].targetType == LogTarget::CONSOLE) {
						cout << formattedMessage;
					} else if (_targetInfo[i].targetType == LogTarget::FILE) {
						*_targetInfo[i].file << formattedMessage;
					}
				}
			}
		}

		delete it->message; //Deallocate message pointer
	}

	for (unsigned int i = 0; i < _targetInfo.size(); i++) {
		if (_targetInfo[i].targetType == LogTarget::FILE) {
			_targetInfo[i].file->close();
		}
	}

	_messageQueue.clear();
}

void CX_Logger::levelForConsole(LogLevel level) {
	bool consoleFound = false;
	for (unsigned int i = 0; i < _targetInfo.size(); i++) {
		if (_targetInfo[i].targetType == LogTarget::CONSOLE) {
			consoleFound = true;
			_targetInfo[i].level = level;
		}
	}

	if (!consoleFound) {
		LoggerTargetInfo consoleTarget;
		consoleTarget.targetType = LogTarget::CONSOLE;
		consoleTarget.level = level;
		_targetInfo.push_back(consoleTarget);
	}
}

/*! Sets the log level for the file with given file name. If the file does not exist, it will be created. 
If the file does exist, it will be overwritten with a warning logged to cerr. 
\param level See the LogLevel enum for valid values.
\param filename Optional. If no file name is given, a file with name generated from a date/time from the start time of the experiment will be used.
*/
void CX_Logger::levelForFile(LogLevel level, string filename) {
	if (filename == "CX_DEFERRED_LOGGER_DEFAULT") {
		filename = "Log file " + CX::Instances::Clock.getExperimentStartDateTimeString("%Y-%b-%e %h-%M-%S %a") + ".txt";
	}
	filename = "logfiles/" + filename;
	filename = ofToDataPath(filename); //Testing
	
	bool fileFound = false;
	for (unsigned int i = 0; i < _targetInfo.size(); i++) {
		if ((_targetInfo[i].targetType == LogTarget::FILE) && (_targetInfo[i].filename == filename)) {
			fileFound = true;
			_targetInfo[i].level = level;
		}
	}

	if (!fileFound) {
		LoggerTargetInfo fileTarget;
		fileTarget.targetType = LogTarget::FILE;
		fileTarget.level = level;
		fileTarget.filename = filename;
		fileTarget.file = new ofFile(); //This is deallocated in the dtor

		fileTarget.file->open(filename, ofFile::Reference, false);
		if (fileTarget.file->exists()) {
			cerr << "Log file already exists with name: " << filename << ". It will be overwritten." << endl;
		}
		
		fileTarget.file->open(filename, ofFile::WriteOnly, false);
		if (fileTarget.file->is_open()) {
			cout << "Log file opened" << endl;
		}
		*fileTarget.file << "CX log file. Created " << CX::Instances::Clock.getDateTimeString() << endl;
		fileTarget.file->close();
		
		_targetInfo.push_back(fileTarget);
	}
}

/*! Sets the log level for the given module.
\param level See the LogLevel enum for valid values.
\param module A string representing one of the modules from which log messages are generated.
*/
void CX_Logger::level (LogLevel level, string module) {
	_moduleLogLevels[module] = level;
}

void CX_Logger::levelForAllModules(LogLevel level) {
	_defaultLogLevel = level;
	for (map<string, LogLevel>::iterator it = _moduleLogLevels.begin(); it != _moduleLogLevels.end(); it++) {
		_moduleLogLevels[it->first] = level;
	}
}

void CX_Logger::setMessageFlushCallback (std::function<void(MessageFlushData&)> f) {
	_flushCallback = f;
}

/*! Set whether or not to log timestamps and the format for the timestamps.
\param logTimestamps Does what it says.
\param format Timestamp format string. See http://pocoproject.org/docs/Poco.DateTimeFormatter.html#4684 for 
documentation of the format. Defaults to %H:%M:%S.%i (24-hour clock with milliseconds at the end).
*/
void CX_Logger::timestamps (bool logTimestamps, string format) {
	_logTimestamps = logTimestamps;
	_timestampFormat = format;
}


stringstream& CX_Logger::verbose (string module) {
	return _log(LogLevel::LOG_VERBOSE, module);
}

stringstream& CX_Logger::notice (string module) {
	return _log(LogLevel::LOG_NOTICE, module);
}

stringstream& CX_Logger::warning (string module) {
	return _log(LogLevel::LOG_WARNING, module);
}

stringstream& CX_Logger::error (string module) {
	return _log(LogLevel::LOG_ERROR, module);
}

stringstream& CX_Logger::fatalError (string module) {
	return _log(LogLevel::LOG_FATAL_ERROR, module);
}

string CX_Logger::_getLogLevelName (LogLevel level) {
	switch (level) {
	case LogLevel::LOG_VERBOSE: return "verbose";
	case LogLevel::LOG_NOTICE: return "notice";
	case LogLevel::LOG_WARNING: return "warning";
	case LogLevel::LOG_ERROR: return "error";
	case LogLevel::LOG_FATAL_ERROR: return "fatal";
	};
	return "";
}



