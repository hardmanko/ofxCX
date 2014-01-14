#include "CX_EntryPoint.h"

void loggerFlushCallback (MessageFlushData& mfd) {
	//cout << "Callback message: " << mfd.message << endl;
}

void setupExperiment (void) {
	Log.levelForFile(LogLevel::LOG_ALL); //Calling levelForFile() without a filename causes a log file with a date/time string filename to be created.
	Log.levelForFile(LogLevel::LOG_ERROR, "Errors only.txt"); //You can have different log levels for different files.
		//You can log to any number of files at once, although there is a linear performance penalty that occurs when flush() is called.

	Log.levelForConsole(LogLevel::LOG_WARNING); //The log level for the console is also independent of the file log levels.

	Log.timestamps(true); //You can log a timestamp for each message

	Log.verbose() << "A verbose detail"; //Send log messages at various levels.
	Log.notice() << "A notice...";
	Log.warning() << "This is a warning";
	Log.error() << "And this is an error";
	Log.fatalError() << "Fatal error!!!";

	Log.level(LogLevel::LOG_WARNING, "myModule"); //Set the log level for the module "myModule"
	Log.level(LogLevel::LOG_NOTICE); //Set the log level for "module-less" messages, i.e. Log.error() << "message";

	Log.error("myModule") << "You can also log to specific modules that have their own log levels.";
	Log.notice("myModule") << "So this message should not appear anywhere.";

	Log.setMessageFlushCallback(loggerFlushCallback); //You can also set up a function that is called every time a message is flushed.

	Log.flush(); //Flush the stored messages to the various logging targets (console and files). This is a BLOCKING operation.
}

bool doingTimeSensitiveStuff = true;

void updateExperiment (void) {
	if (doingTimeSensitiveStuff) {
		//Time sensitive stuff...
		doingTimeSensitiveStuff = false;
	} else {
		Log.flush(); //Only flush() when not doing time sensitive stuff
		doingTimeSensitiveStuff = true;
	}
}