#include "CX_DeferredLogger.h"

using namespace CX;

CX_Logger CX::Log;

CX_Logger::CX_Logger (void) :
	timestamps(false)
{
	levelForConsole(LogLevel::LOG_ALL);
}

CX_Logger::~CX_Logger (void) {
	flush();
}

//This function is called at the start of a new messsage. It also concludes the previous message (if any).
stringstream& CX_Logger::_log (LogLevel level, string module) {
	
	_messageQueue.push_back( LogMessage(level, module) );
	_messageQueue.back().message = new stringstream; //Manually allocated: Must deallocate later. Cannot allocate in LogMessage ctor for some reason.

	if (timestamps) {
		//_currentMessage.timestamp = timestamp; //Avoids timestamp formatting at call site.

		//*_currentMessage.ss << timestampString;
		//CX::getDateTimeString
	}

	return *(_messageQueue.back().message);
}

void CX_Logger::flush (void) {

	for (unsigned int i = 0; i < _targetInfo.size(); i++) {
		if (_targetInfo[i].targetType == LogTarget::FILE) {
			_targetInfo[i].file.open(_targetInfo[i].filename, ofFile::Append, false);
		}
	}

	for (auto it = _messageQueue.begin(); it != _messageQueue.end(); it++) {

		//ofNotifyEvent(this->messageFlushEvent, LogEventData( it->message->str(), it->level, it->module ) );

		string logName = _getLogLevelName(it->level);
		logName.append( max<int>((int)(7 - logName.size()), 0), ' ' ); //Pad out names to 7 chars
		string formattedMessage = "[ " + logName + " ] ";

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
						_targetInfo[i].file << formattedMessage;
					}
				}
			}
		}

		delete it->message; //Deallocate message pointer
	}

	for (unsigned int i = 0; i < _targetInfo.size(); i++) {
		if (_targetInfo[i].targetType == LogTarget::FILE) {
			_targetInfo[i].file.close();
		}
	}

	_messageQueue.clear();
}

/*
void CX_Logger::to (LogTarget target, string filename) {
	_logTarget = target;
	if (target == LogTarget::FILE || target == LogTarget::CONSOLE_AND_FILE) {
		//If no filename is given, generate a file name based on date/time.
		//if (filename == "") {
		//string filename = CX::Clock.getDateTimeString();
		//	}
		//Check that it is a valid file and give error if not.
		//_outputFile.close();
		//_outputFile.open( ofToDataPath(filename), ofFile::Reference, false );
		_logFileName = filename;
	}
}
*/

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
	//If an output target with the given filename is not found, create it with the given level.
	//If it is found, set the level to the new level.
	//If filename == "", use the default filename as the target. Use CX::Clock.getExperimentStartDateTime() to create the file name.
	if (filename == "CX_DEFERRED_LOGGER_DEFAULT") {
		//filename = CX::Clock.getExperimentStartDateTime();
	}
	
	bool fileFound = false;
	for (unsigned int i = 0; i < _targetInfo.size(); i++) {
		if (_targetInfo[i].targetType == LogTarget::FILE) {
			fileFound = true;
			_targetInfo[i].level = level;
		}
	}

	if (!fileFound) {
		LoggerTargetInfo fileTarget;
		fileTarget.targetType = LogTarget::FILE;
		fileTarget.level = level;
		fileTarget.filename = filename;

		if (!fileTarget.file.open(filename, ofFile::Reference, false)) {
			//error...
			//return false;
		}
		fileTarget.file.close();
		
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
	return _log(LogLevel::ERROR, module);
}

stringstream& CX_Logger::fatalError (string module) {
	return _log(LogLevel::FATAL_ERROR, module);
}

string CX_Logger::_getLogLevelName (LogLevel level) {
	switch (level) {
	case LogLevel::VERBOSE: return "verbose";
	case LogLevel::NOTICE: return "notice";
	case LogLevel::WARNING: return "warning";
	case LogLevel::ERROR: return "error";
	case LogLevel::FATAL_ERROR: return "fatal";
	};
	return "";
}



