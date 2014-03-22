Program Model {#programModel}
=============

Program Flow
------------
One of the foundational aspects of CX is the design of the overall program flow, which includes things such as how responses are collected, how stimuli are drawn to the screen, and other similar concepts. The best way to learn about program flow is to examine the \ref examplesAndTutorials "examples". The examples cover most of the critical topics and introduce the major components of CX.

The most important thing to understand is that in CX, nothing happens that your code does not explicitly ask for, with the exception of a small amount of setup, which is discussed below (see Pre-experiment Setup). For example, CX does not magically collect and timestamp user responses for you. Your code must poll for user input in order to get timestamps for input. This is explained more in the input section. In CX, there is no code running in the background that makes everything work out for your experiment, you have to design your experiment in such a way that you are covering all of your bases. That said, CX is specifically designed to make doing that as easy and painless as possible, while still giving you as much control over your experiment as is reasonably possible.

Internals/Attributions
----------------------
CX is not a monolithic entity. It is based on a huge amount of open source software written by many different authors over the years. Clearly, CX is based on openFrameworks, but openFramworks itself is based on many different libraries. Window handling, which involves creating a window that can be rendered to and receiving user input events from the operating system, is managed by GLFW (http://www.glfw.org/). The actual rendering is visual stimuli is done using OpenGL (http://www.opengl.org/), which is wrapped by several openFrameworks abstractions (e.g. ofGLProgrammableRenderer at a lower level, e.g. ofPath at the level at which a typical user would use).

Audio is processed in different ways depending on the type of audio player used. CX_SoundBufferPlayer and CX_SoundBufferRecorder wrap CX_SoundStream which wraps RtAudio (https://www.music.mcgill.ca/~gary/rtaudio/). If you are using ofSoundPlayer, depending on your operating system it might eventually use FMOD on Windows or OSx (http://www.fmod.org/; although the openFrameworks maintainers are considering moving away from FMOD) or OpenAL on Linux (http://en.wikipedia.org/wiki/OpenAL). However, you should check that this information is correct.

There are other libraries that are a part of openFrameworks that I am not as familiar with, including Poco (http://pocoproject.org/), which provides a variety of very useful utility functions and networking, FreeType (http://www.freetype.org/) which does font rendering, and many others. 

CX would not have been possible without the existence of these high-quality open-source projects.

Overriding openFrameworks
-------------------------
Although CX is technically an addon to openFrameworks, there are a number of ways in which CX hijacks normal oF functionality in order to work better. As such, you cannot assume that all oF functionality is available to you.

Generally, drawing visual stimuli using oF classes and functions is fully supported. See the renderingTest example to see a pletora of ways to put things on the screen.

Audio output using ofSoundPlayer is supported, although no timing guarantees are made. Prefer CX::CX_SoundBufferPlayer.

The input events (e.g. ofEvents().mousePressed) technically work, but with two serious limitations. 1) The events only fire when CX_InputManager::pollEvents() is called (which internally calls glfwPollEvents() to actually kick off the events firing). 2) The standard oF events do not have timestamps, which limits their usefulness.

The following functions' behavior is superseded by functionality provided by CX_Display (see also CX::Instances::Display): 
ofGetFrameNum() is replaced by CX_Display::getFrameNumber()
ofGetLastFrameTime() is replaced by CX_Display::getLastSwapTime()

The following functions do nothing: ofGetFrameRate(), ofSetFrameRate(), ofGetTargetFrameRate()

A variety of behaviors related to ofBaseApp do not function because CX is not based on a class derived from ofBaseApp nor does it use ofRunApp() to begin the program. For example, a standard oF app class should have steup(), update(), and draw() functions that will be called by oF during program execution. CX has a different model that does not force object-orientation (at least at some levels).


Pre-experiment Setup
--------------------

There is very little that CX does without you asking for it. The one major exception is pre-experiment setup, in which a number of basic operations are performed in order to set up a platform on which the rest of the experiment can run. The most significant step is to open a window and set up the OpenGL rendering environment. The main pseudorandom number generator (CX::Instances::RNG) is seeded. The logging system is prepared for use. The main clock (CX::Instances::Clock) is prepared for use.


Input Timing
------------
The way user input is handled by CX is easily explained by giving the process of receiving a mouse click. Assume that a CX program is running in a window. The user clicks inside of the window. At this point, the operating system (or at least the windowing subsystem of the operating system, but I will choose to conflate them) detects that the click has occured and notes that the location of the click is within the window. The operating system then attempts to tell the program that a mouse click has occured. In order to be notified about input events like mouse click, the program has previously set up a message queue for incoming messages from the operating system. The OS puts the mouse event into the message queue. In order for the program to find out about the message it needs to check to message queue. This is what happens when CX::CX_InputManager::pollEvents() is called: The message queue is checked and all messages in the queue are processed, given timestamps, and routed to the next queue (e.g. the message queue in CX::CX_Mouse that is accessed with CX::CX_Mouse::availableEvents() and CX::CX_Mouse::getNextEvent()). The timestamps are not given by the operating system*, so if pollEvents is not called regularly, input events will be received and everything will appear to be working correctly, but the timestamps will be wrong.

Of course, the actual process extends all the way back to the input device itself. The user presses the button and the microcontroller in the input device senses that a button has been pressed. It places this button press event into its outgoing message queue. At the next polling interval (typically 1 ms), the USB host controller on the computer polls the device for messages, discovers that a message is waiting and copies the message to the computer. At some point, the operating system checks to see if the USB host controller has received messages from any devices. It discovers the message and moves the message into the message queue of the program. At each step in which the message moves from one message queue to the next, the data contained in the message likely changes a little. At the start in the mouse, the message might just be "button 1 pressed". At the next step in the USB host controller, the message might be "input device 1 (type is mouse) button 1 pressed". Once the operating system gets the message it might be "mouse button 1 pressed while cursor at (367, 200) relative to clicked window". Eventually, the message gets into the message queue that users of CX work with, in CX::CX_Mouse, for example.

This process sounds very long and complicated, suggesting that it might take a long time to complete, throwing off timing data. That is true: Input timing data collected by CX is not veridical, there are invariably delays, including non-systematic delays. However, there are several steps in the process that no experiment software can get around, so the problems with timing data are not unique to CX. It might be possible to write a custom driver for the mouse or keyboard that allows the software to bypass the operating system's message queue, but it is very difficult to avoid the USB hardware delays, which can be on the order of milliseconds for many kinds of standard input devices. The next layer of the problem is that we are really interested in response time to a specific stimulus, but the time at which the stimulus was actually presented may be misreported by audio or video hardware/software, so even if the response timestamp had no error whatsoever, when it is compared with the stimulus presentation time, the respones latency would be wrong due to errors in measures stimulus presentation time. Based on this large set of problems with collecting accurate response latency data, it is my firm belief that the only way to accurately measure response latency is with a button box that measures actual stimulus onset time with a light or sound sensor and also measures the time of a button press or other response. If you don't use such a system, the expectation is that you simply allow any error in response latencies to be dealt with statistically. Typically, any systematic error in response times will be subtracted out when conditions are compared with one another. Any random error will simply slightly inflate the estimated variance, but probably not to any meaningful extent.

If you would like to learn more about the internals of how input is handled in CX, you can see how GLFW and openFramworks manage input by examining the source code in the respective repositories.

*Technically, on Windows the messages that are given to a program do have a timestamp. However, the documentation doesn't actually say what the timestamp represents. My searching turns up the suggestion that it is a timestamp in milliseconds from system boot, but that the timestamp is set using the GetTickCount function, which typically has worse than 10 ms precision. This makes the timestamp attached to the message of very little value. See this page for documentation of what information comes with a Windows message: http://msdn.microsoft.com/en-us/library/windows/desktop/ms644958%28v=vs.85%29.aspx. The only page on which I actually found a definition of what the time member stores is this page http://msdn.microsoft.com/en-us/library/aa929818.aspx, which gives information pertaining to Windows Mobile 6.5, which is an obsolete smartphone operating system.


Stimulus Timing
---------------

Although the kinds of error introduced into response time data can often be dealt with statistically, errors in stimulus presentation can be more serious. For example, if a visual stimulus is systematically presented for an extra frame throughout an experiment, then the method of the experiment has been altered without the experimenter learning about the alteration. Even if the extra frame does not always happen, on average participants are seeing more of that stimulus than they should be. An error on the magnitude of an extra frame is nearly impossible to detect by eye in most cases, so it is important that there is some way to detect errors in stimulus presentation. The primary method of presenting time-locked visual stimuli is the CX_SlidePresenter. It has built in error-detection features that pick up on certain kinds of errors. Information about presentation errors can be found by using CX::CX_SlidePresenter::checkForPresentationErrors().

Although it is nice to be made aware of errors when they occur, it is better to not have the errors happen in the first place. For this reason, stimulus presentation in CX is designed around avoiding errors. For visual stimuli, the \ref CX::CX_SlidePresenter "CX_SlidePresenter" provides a very easy-to-use way to present visual stimuli. Because the interface is so simple, user error is minimized. The backend code of the CX_SlidePresenter is designed to minimize the potential for timing errors by carefully tracking the passage of time, monitor refreshes, and timing of stimulus rendering. 

On the audio front, CX provides the \ref CX::CX_SoundBufferPlayer "CX_SoundBufferPlayer", which plays CX::CX_SoundBuffer "CX_SoundBuffers". If several sounds are to be presented in a time-locked sequence, playing the sounds individually at their intended onset time can result in unequal startup delays for each sound, but if all of the sounds are combined together into a single audio buffer this possibility is eliminated. CX_SoundObjects are designed to make combining multiple sound stimuli together easy, which helps to prevent timing errors that could have otherwise occurred between sounds. CX also includes \ref CX::CX_SoundStream "CX_SoundStream", which provides a method for directly acessing and manipulating the contents of audio buffers that are received from or sent to audio hardware.


