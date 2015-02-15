/* This example shows various features of the logging system in CX.

One feature is of the logging system is filtering of log messages, which is done based on
the severity of the messages, the module that created the message, and the output targets 
to which messages are logged. All messages that are logged are assigned a severity, from 
a verbose detail to a fatal error. When messages are logged, an optional string giving 
the name of the module that logged the message may be provided. Filtering is first done 
based on which module created the message (i.e. the message source). For each module, 
messages can be ignored if they are less severe than a set level. For the second layer of 
filtering, output targets, like the console or logfiles, can be set to ignore messages 
that are less severe than a certain level.

This two-level filtering can be used in complex ways, as in this example, to carefully control
what is logged. However, logging everything to at least one log file is usually a good idea, 
even if the console is kept clear for the most important messages.

By default, CX does not create a log file.
*/

#include "CX.h"

//The body of this function is commented out so as to not interfere with reading what is in the console.
//You can uncomment it to see the result of this callback.
void loggerFlushCallback (CX_Logger::MessageFlushData& mfd) {
	//cout << "Callback message: " << mfd.message << endl;
}

void runExperiment (void) {

	Log.levelForFile(CX_Logger::Level::LOG_ERROR, "Errors only.txt"); //You can have different log levels for different files.
		//You can log to any number of files at once, although for each additional file there is a linear performance 
		//cost that occurs when Log.flush() is called.

	//Calling levelForFile() without a filename causes a log file with a date/time string filename to be created.
	Log.levelForFile(CX_Logger::Level::LOG_ALL);

	Log.levelForConsole(CX_Logger::Level::LOG_WARNING); //The log level for the console is also independent of the file log levels.

	Log.timestamps(true); //You can log a timestamp for each message, with an optional time format 
		//argument that defaults to hours:minutes:seconds.milliseconds.

	//Send log messages at various levels of importance.
	Log.verbose() << "A verbose detail that you probably don't need to know.";
	Log.notice() << "A notice about routine operation of the system.";
	Log.warning() << "This is a warning.";
	Log.error() << "And this is an error.";
	Log.fatalError() << "Fatal error!!!";

	Log.level(CX_Logger::Level::LOG_NOTICE, ""); //Set the log level for "module-less" messages (i.e. module is the empty string), like those above.
	//This means that the verbose message above won't be sent to any of the outputs.

	//By giving a string to the logging functions, it sets that string as the name of the module that
	//the message came from. The module name is logged along with messages and has implications for
	//message filtering.
	Log.error("myModule") << "You can also log to specific named modules that have their own log levels.";
	
	//Set the log level for the module "myModule" to ignore anything less than a warning.
	Log.level(CX_Logger::Level::LOG_WARNING, "myModule");

	Log.notice("myModule") << "This message should not appear anywhere because it is filtered out.";


	//You can also set up a function that is called every time a message is flushed.
	//The body of loggerFlushCallback is commented out above, so you won't see a result from this unless it is uncommented.
	Log.setMessageFlushCallback(loggerFlushCallback);


	//By default, all messages logged using the oF logging system (everything internal to oF is logged that way) are 
	//routed into Log, from which they can be flushed.
	ofLogWarning("using ofLogWarning") << "You have been warned about oF logging!";

	ofLogError("using ofLogError", "%d plus %f is %f", 50, 0.5, 50 + 0.5); //You can also use C-style formatting with oF logging.
		//If you want openFrameworks messages to be logged normally (i.e. not by CX::Instances::Log.flush()), you can call 
		//ofLogToConsole() or ofLogToFile(), although this is not recommended becuase there is no way to control when messages are 
		//flushed when using the standard oF logging. See the documentation on oF logging at 
		//http://openframeworks.cc/documentation/utils/ofLog.html for more information about oF logging.


	//Flush the stored messages to the various logging targets (console and files). 
	//This is a potentially blocking operation, depending on the number of stored log messages that haven't been flushed.
	Log.flush();

	Log.levelForConsole(CX_Logger::Level::LOG_ALL); //Now, log messages of every kind to the console

	//You can also set the log level for all modules. This allows you to set the log level for all modules, 
	//then selectively set a different log level for other modules if, e.g. you are trying to debug a specific 
	//module and want to only see output from it. In that case, you could do:
	Log.levelForAllModules(CX_Logger::Level::LOG_NONE);
	Log.level(CX_Logger::Level::LOG_ALL, "myTargetModule");


	Log.notice("myTargetModule") << "A special message, just for you!";
	Log.fatalError("ReactorCore") << "Meltdown imminent!!! Too bad you won't get this...";

	Log.flush();

	Log.levelForAllModules(CX_Logger::Level::LOG_ALL);


	//If instead of a logged message for the more serious errors, you want an exception:
	Log.levelForAllExceptions(CX_Logger::Level::LOG_FATAL_ERROR);

	Log.warning() << "Almost out of memory.";
	try {
		Log.fatalError() << "Totally out of memory.";
	} catch (std::runtime_error& e) {
		//The type of the exception that is thrown is a std::runtime_error, so that's what we will catch.
		Log.notice() << "A fatal error occured and the exception was caught.";
		Log.notice() << "The exception information is: \"" << e.what() << "\"";
	}
	//Note that the error that triggered the exception was logged normally, so if you catch
	//the exception you don't need to do anything extra to get the message.

	//The exception will be thrown from near the call site of the logging call that generated the exception,
	//which means that you get a useful stack trace.


	//The function Util::checkOFVersion logs with module named "CX::Util::checkOFVersion", so lets enable exceptions for that module
	Log.levelForExceptions(CX_Logger::Level::LOG_ALL, "CX::Util::checkOFVersion");

	//We should get an exception within this function (this is not a supported version of openFrameworks)
	//which should allow us to examine the call stack. It's commented out so as not to break the rest of the example.
	//Util::checkOFVersion(0, 1, 1, true);


	Log.flush();

	//Press any key to exit.
	Input.Keyboard.waitForKeypress(-1);
}