Blocking Code {#blockingCode}
=============

Blocking code is code that either takes a long time to complete or that waits until some event occurs before allowing code execution to continue. An example of blocking code that waits:

\code{.cpp}
do {
	Input.pollEvents();
} while (Input.Keyboard.availbleEvents() == 0);
\endcode

This code waits until the keyboard has been used in some way. No code past it can be executed until the keyboard is used, which could take a long time. Any code that blocks while waiting for a human to do something is blocking. Additionally, code that waits on network resources (e.g. loading a file from an external server) is also typically blocking. Also, loading large files, such as long audio files or video files from the hard drive can be blocking. This is why you should try to load all of your stimuli into RAM at the beginning of the experiment rather than loading them from the hard drive just before they should be presented.

An example of blocking code that takes a long time (or at least could take a long time) is

\code{.cpp}
vector<double> d = CX::Util::sequence<double>(0, 1000000, 0.033);
\endcode

which requires the allocation of about 300 MB of RAM to store the sequence of numbers from 0 to 1000000 in increments of 0.033. This code doesn't wait for anything to happen, it just takes a long time to execute. It is important to stress that the vast majority of code that does not wait on input or output is not blocking, even if it is doing something very complex, because modern computers are extremely fast. Allocating a lot of memory can take a long time. Also, applying an algorithm to a large amount of data can take a long time. However, most of the things usually done in psychology experiments take very little time. If you are curious about the amount of time being taken to execute a piece of code, CX provides CX::Util::CX_SegmentProfiler and CX::Util::CX_LapTimer to help with measuring time taken. See the documentation for those classes for information on usage.

If you are trying to collect accurate response data or present stimuli at specific times, one of the worst things you can do is have a section of blocking code that runs while a time-sensitive task is taking place. The main concern with blocking code is that while the code is running some critical timing period passes and a stimulus is not presented or a response is collected but not timestamped correctly. The question, then, is how long can code run before it is considered blocking code. As a rough guideline, if a section of code takes longer than about 1 ms to run, it has the potential to disrupt timing meaningfully. 

Using blocking code is not a cardinal sin and there are times when using blocking code is acceptable. However, blocking code should not be used when trying to present stimuli or when responses are being made. The reason for this is that CX expects to be able to repeatedly check information related to stimulus presentation and input at very short intervals (at least every millisecond), but that cannot happen if a piece of code is blocking. There is of course an exception to the "no blocking while waiting for responses" rule, which is when your blocking code is doing nothing but waiting for a response and constantly polling for user input. For example, the following code waits until any response is made:

\code{.cpp}
while(!Input.pollEvents())
	;
//Process the inputs.
\endcode

As long as you aren't also trying to present stimuli, by constantly polling for input, the code will notice input as soon as possible and the timestamp for the input event will be very accurate.
