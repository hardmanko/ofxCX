#include "CX.h"

//The body of this function is commented out so as to not interfere with reading what is in the console.
//You can uncomment it to see the result of this callback.
void loggerFlushCallback (CX_MessageFlushData& mfd) {
	//cout << "Callback message: " << mfd.message << endl;
}

void runExperiment (void) {
	Log.levelForFile(CX_LogLevel::LOG_ALL); //Calling levelForFile() without a filename causes a log file with a 
		//date/time string filename to be created.

	Log.levelForFile(CX_LogLevel::LOG_ERROR, "Errors only.txt"); //You can have different log levels for different files.
		//You can log to any number of files at once, although for each additional file there is a linear performance 
		//cost that occurs when flush() is called.

	Log.levelForConsole(CX_LogLevel::LOG_WARNING); //The log level for the console is also independent of the file log levels.

	Log.timestamps(true); //You can log a timestamp for each message, with an optional time format 
		//argument that defaults to hours:minutes:seconds.milliseconds.

	Log.verbose() << "A verbose detail"; //Send log messages at various levels.
	Log.notice() << "A notice...";
	Log.warning() << "This is a warning";
	Log.error() << "And this is an error";
	Log.fatalError() << "Fatal error!!!";

	Log.level(CX_LogLevel::LOG_WARNING, "myModule"); //Set the log level for the module "myModule"
	Log.level(CX_LogLevel::LOG_NOTICE); //Set the log level for "module-less" messages, i.e. Log.error() << "message";	

	Log.error("myModule") << "You can also log to specific modules that have their own log levels.";
	Log.notice("myModule") << "So this message should not appear anywhere.";

	Log.setMessageFlushCallback(loggerFlushCallback); //You can also set up a function that is called every time a message is flushed.
		//The body of loggerFlushCallback is commented out above, so you won't see a result from this unless it is uncommented.

	//By default, all messages logged using the oF logging system (everything internal to oF is logged that way) are 
	//routed into Log, from which they can be flushed.
	ofLogWarning("using ofLogWarning") << "You have been warned about oF logging!";

	ofLogError("using ofLogError", "%d plus %f is %f", 50, 0.5, 50 + 0.5); //You can also use C-style formatting with oF logging.
		//If you want openFrameworks messages to be logged normally (i.e. not by CX::Instances::Log.flush()), you can call 
		//ofLogToConsole() or ofLogToFile(), although this is not recommended becuase there is no way to control when messages are 
		//flushed when using the standard oF logging. See the documentation on oF logging at 
		//http://openframeworks.cc/documentation/utils/ofLog.html for more information about oF logging.

	Log.flush(); //Flush the stored messages to the various logging targets (console and files). 
		//This is a potentially blocking operation, depending on the number of stored log messages that haven't been flushed.

	Log.levelForConsole(CX_LogLevel::LOG_ALL); //Log everything to the console

	//You can also set the log level for all modules. This allows you to set the log level for all modules, 
	//then selectively set a different log level for other modules if, e.g. you are trying to debug a specific 
	//module and want to only see output from it. In that case, you could do:
	Log.levelForAllModules(CX_LogLevel::LOG_NONE);
	Log.level(CX_LogLevel::LOG_ALL, "myTargetModule");

	Log.notice("myTargetModule") << "A special message, just for you!";
	Log.fatalError("ReactorCore") << "Meltdown imminent!!! To bad you won't get this...";

	Log.flush();

	Input.setup(true, false);
	while (!Input.pollEvents())
		;
}