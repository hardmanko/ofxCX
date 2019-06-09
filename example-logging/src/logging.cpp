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

// A function can be set up to listen as messages are flushed, which allows you
// to work with the raw message rather than a string representation of it.
//
// This sample function waits for a message from the "flushListener" module.
void loggerFlushListener(const CX_LogMessage& msg) {

	if (msg.module == "flushListener") {
		cout << ">>> The flushListener module logged a message at " << msg.timestamp.seconds() <<
			" and that message was just flushed." << endl;
	}
}

void runExperiment (void) {

	// You can log a timestamp for each message, with an optional time format 
	// argument that defaults to "hours:minutes:seconds.milliseconds".
	// Here, I've set the format to "minutes:seconds.milliseconds.micrseconds".
	Log.timestamps(true, "%M:%S.%m.%u");


	////////////////////////////
	// Set Up Logging Targets //
	////////////////////////////

	// At the beginning of an experiment, you can set up where logged messages
	// will be printed. By default, notices and above are printed to the console.

	// You can log to any number of files at once, each with its own log level.
	// Let's log errors and worse (errors and fatal errors) to their own file.
	Log.levelForFile(CX_Logger::Level::LOG_ERROR, "Errors only.txt");

	// We also want a file in which all messages are logged. Calling levelForFile() 
	// without a filename causes a log file with a date/time string filename to be created.
	Log.levelForFile(CX_Logger::Level::LOG_ALL);
	
	// The log level for the console is independent of the file log levels.
	// Set the console to display all messages that are a warning or worse (warning, error, fatal error).
	Log.levelForConsole(CX_Logger::Level::LOG_WARNING); 

	
	///////////////////
	// Basic Logging //
	///////////////////

	// The basic logging function is log(), for which a logging level must be provided.
	// Then you use standard c++ stream operators to stream data
	Log.log(CX_Logger::Level::LOG_NOTICE) << "Log.log() logged a notice.";
	// You don't need to use endl: Logged messages have line endings added when printing.

	// For brevity, you can use CX_Logger::operator() to log a notice
	Log() << "Using operator(), the message should be logged as a notice.";

	// Using c++ stream operators, it is easy log many data types, not just strings.
	int number = 65;
	CX_Millis time = 123.456;
	Log() << "Number: " << number << ", Time: " << time;


	// The logging functions that I use the most are named for their logging level.
	Log.verbose() << "System temperature is " << 273 + 600 << " K, which is nominal.";
	Log.notice() << "Temps are getting high, but within normal range.";
	Log.warning() << "Safety triggered: Shutting down.";
	Log.error() << "Shutdown failed. Temperatures increasing.";
	Log.fatalError() << "Now we're dead.";

	// The messages above have been stored in the message queue of the logger,
	// but have not yet been flushed to the logging targets (the console and files).
	// Flush queued messages to the logging targets with flush().
	Log.flush();
	
	// Once messages have been flushed to logging targets, they are automatically
	// deleted from the logger.
	
	// It takes time to flush messages to logging targets, potentially much longer
	// than it takes to simply store the message in the logger. This means that flush()
	// should be thought of as a potentially blocking function and should not be
	// called in timing-critical periods. 
	// I typically call flush() once between each trial.


	///////////////////////
	// Module Log Levels //
	///////////////////////
	
	// By providing a string argument to the logging functions, it sets that string as the name of the module that
	// the message came from. The module name is logged along with messages.
	Log.error("myModule") << "You can also log to named modules that have their own log levels.";

	// A message from a module can be filtered out if the message log level is less than that module's log level.
	// Set the log level for the module "myModule" to ignore anything less than a warning with levelForModule() .
	Log.levelForModule(CX_Logger::Level::LOG_WARNING, "myModule");

	// This message should be filtered because its level is notice, but myModule is configured to only have warnings and above logged.
	Log.notice("myModule") << "This message should not appear anywhere because it is filtered out.";

	
	// If no module name is provided, the module name defaults to the empty string.
	Log.verbose() << "This message has no module.";

	// The log level for "module-less" messages can be set by using the empty string for the module name.
	Log.levelForModule(CX_Logger::Level::LOG_NOTICE, "");


	// Reset message filtering: Log messages of every kind to the console
	Log.levelForConsole(CX_Logger::Level::LOG_ALL);

	// You can set the log level for all modules at once. This allows you to set the log level for all modules, 
	// then selectively set a different log level for specific modules.
	// For example, if you are debugging a specific buggy module you might want to only see output from that module.
	// In that case, you could do:
	Log.levelForAllModules(CX_Logger::Level::LOG_NONE); // Log nothing from any module
	Log.levelForModule(CX_Logger::Level::LOG_ALL, "myBuggyModule"); // Except everything from "myBuggyModule"

	Log.notice("myBuggyModule") << "A special message, just for you!";
	Log.fatalError("reactorCore") << "Meltdown imminent!!! Too bad you won't get this.";

	Log.flush();

	// Reset to logging all modules' messages
	Log.levelForAllModules(CX_Logger::Level::LOG_ALL);


	/////////////////////////////////
	// Raw Message Access on Flush //
	/////////////////////////////////
	
	// When messages are flushed, you can set up your own logging method by setting
	// up a callback function to listen to the flush event of the logger.
	ofAddListener(Log.flushEvent, loggerFlushListener);

	// Log a message to the flush listener function.
	Log.notice("flushListener") << "This notice is just for the flush listener.";
	// (There is nothing special about the module name "flushListener", 
	// you can do this with any module name.)

	// For message flush events, no filtering is performed, so even if the module
	// log level for the flushListener is LOW_NONE, the listener still gets the message.
	Log.levelForModule(CX_Logger::Level::LOG_NONE, "flushListener");

	Log.flush();
	

	////////////////////////////
	// OpenFrameworks Logging //
	////////////////////////////

	// By default, all messages logged using the oF logging system (that oF uses internally) are 
	// routed into Log, from which they can be flushed.
	ofLogWarning("using ofLogWarning") << "You have been warned about oF logging!";

	// You can also use C-style formatting by using a version of the oF logging functions.
	ofLogError("using ofLogError", "%d plus %f is %f", 50, 0.5, 50 + 0.5);

	// To stop Log from capturing oF log messages so that those 
	// messages get logged in the normal oF way, call:
	//
	// Log.captureOFLogMessages(false);
	//
	// This is not recommended for final timing-critical software because oF logging
	// flushes messages immediately, which is undesireable during timing-critical sections.
	//
	// For more information about oF logging, see its documentation here: 
	// http://openframeworks.cc/documentation/utils/ofLog.html

	Log.flush();

	//////////////////////////////////////////
	// Debugging with Exceptions (Advanced) //
	//////////////////////////////////////////

	// Most kinds of bugs can be solved with message logging, but sometimes it is also helpful
	// to have an exception be thrown when certain kinds of message are logged, such as fatal errors
	// or messages from certain modules.
	//
	// The exception will be thrown from near the call site of the logging call that generated the exception,
	// which means that you get a stack trace if your debugger breaks on unhandled exceptions.
	// Having an interactive debugger and stack trace from the logging call site can be very helpful when debugging.

	// Make fatal errors for any module trigger an exception:
	Log.levelForAllExceptions(CX_Logger::Level::LOG_FATAL_ERROR);

	// Log some messages. The warning won't throw.
	Log.warning() << "Almost out of memory.";

	try {

		// This fatal error should throw here.
		Log.fatalError() << "Totally out of memory.";

		// So catch it. CX_Logger throws exceptions of type std::runtime_error.
	} catch (std::runtime_error& e) {

		Log.notice() << "A fatal error occured, triggered an exception, and the exception was caught.";
		Log.notice() << "The exception information is: \"" << e.what() << "\"";
	}
	// Note that the error that triggered the exception was logged normally, so if you catch
	// the exception you don't need to do anything extra to get the message.

	// If you want exceptions to trigger when messages are logged, you probably don't want to put
	// exception handling code (try catch blocks) around every message that you log.
	// At least in Visual Studio, uncaught exceptions trigger a breakpoint, giving you can interactive
	// debugger and stack trace to help you examine the cause of the exception.
	//
	//Log.fatalError() << "This fatal error is not in a try catch block, its exception will not be caught.";
	//
	// It's commented out so that this example doesn't throw an exception before completing.

	Log.flush();



	// Wait for user to press any key to exit.
	Input.Keyboard.waitForKeypress(-1);
}