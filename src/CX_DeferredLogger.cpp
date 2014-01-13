#include "CX_DeferredLogger.h"

using namespace CX;

CX_Logger CX::Instances::Log;

CX_Logger::CX_Logger (void) :
	timestamps(false)
{
	levelForConsole(LogLevel::LOG_ALL);
}

CX_Logger::~CX_Logger (void) {
	flush();
	for (unsigned int i = 0; i < _targetInfo.size(); i++) {
		_targetInfo[i].file->close();
		delete _targetInfo[i].file;
	}
}

//This function is called at the start of a new messsage.
stringstream& CX_Logger::_log (LogLevel level, string module) {
	
	_messageQueue.push_back( LogMessage(level, module) );
	_messageQueue.back().message = new stringstream; //Manually allocated: Must deallocate later. Cannot allocate in LogMessage ctor for some reason.

	if (timestamps) {
		_messageQueue.back().timestamp = CX::Instances::Clock.getDateTimeString("%H:%M:%S");
	}

	return *(_messageQueue.back().message);
}

void CX_Logger::flush (void) {

	for (unsigned int i = 0; i < _targetInfo.size(); i++) {
		if (_targetInfo[i].targetType == LogTarget::FILE) {
			bool opened = _targetInfo[i].file->open(_targetInfo[i].filename, ofFile::Append, false);
			if (!opened) {
				cerr << "File " << _targetInfo[i].filename << " not opened for logging." << endl;
			}
		}
	}

	for (auto it = _messageQueue.begin(); it != _messageQueue.end(); it++) {
		LogEventData dat( it->message->str(), it->level, it->module );
		ofNotifyEvent(this->messageFlushEvent, dat );

		string logName = _getLogLevelName(it->level);
		logName.append( max<int>((int)(7 - logName.size()), 0), ' ' ); //Pad out names to 7 chars
		string formattedMessage;
		if (timestamps) {
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

void CX_Logger::levelForFile(LogLevel level, string filename) {
	if (filename == "CX_DEFERRED_LOGGER_DEFAULT") {
		filename = "Log file " + CX::Instances::Clock.getExperimentStartDateTimeString("%Y-%b-%e %h-%M-%S %a") + ".txt";
	}

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
			cout << "File opened" << endl;
		}
		*fileTarget.file << "CX log file. Created " << CX::Instances::Clock.getDateTimeString() << endl;
		fileTarget.file->close();
		
		_targetInfo.push_back(fileTarget);
	}
}

void CX_Logger::level (LogLevel level, string module) {
	_moduleLogLevels[module] = level;
}


stringstream& CX_Logger::verbose (string module) {
	return _log(LogLevel::VERBOSE, module);
}

stringstream& CX_Logger::notice (string module) {
	return _log(LogLevel::NOTICE, module);
}

stringstream& CX_Logger::warning (string module) {
	return _log(LogLevel::WARNING, module);
}

stringstream& CX_Logger::error (string module) {
	return _log(LogLevel::LOG_ERROR, module);
}

stringstream& CX_Logger::fatalError (string module) {
	return _log(LogLevel::FATAL_ERROR, module);
}

string CX_Logger::_getLogLevelName (LogLevel level) {
	switch (level) {
	case LogLevel::VERBOSE: return "verbose";
	case LogLevel::NOTICE: return "notice";
	case LogLevel::WARNING: return "warning";
	case LogLevel::LOG_ERROR: return "error";
	case LogLevel::FATAL_ERROR: return "fatal";
	};
	return "";
}



