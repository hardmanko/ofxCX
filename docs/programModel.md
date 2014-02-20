
Program Model {#programModel}
=============

One of the foundational aspects of CX is the design of the overall program flow, which includes things such as how responses are collected, how stimuli are drawn to the screen, and other similar concepts.

The most important thing to understand is that in CX, nothing happens that your code does not explicitly ask for, with the exception of a small amount of setup, which is discussed below. For example, CX does not magically collect and timestamp user responses for you. Your code must poll for user input in order to get timestamps for input. This is explained more in the input section. In CX, there is no code running in the background that makes everything work out for your experiment, you have to design your experiment in such a way that you are covering all of your bases. That said, CX is specifically designed to make doing that as easy and painless as possible, while still giving you as much control over your experiment as is reasonably possible.


Internals
---------
CX is not a monolithic entity. It is based on a huge amount of open source software written by many different authors over the years. Clearly, CX is based on openFrameworks, but openFramworks itself is based on many different libraries. Window handling, which involves creating a window that can be rendered to and receiving user input events from the operating system, is managed by GLFW (http://www.glfw.org/). The actual rendering is visual stimuli is done using OpenGL (http://www.opengl.org/), which is wrapped by several openFrameworks abstractions (e.g. ofGLProgrammableRenderer at a lower level, e.g. ofPath at the level at which a typical user would use).

Audio is processed in different ways depending on the type of audio player used. CX_SoundObjectPlayer wraps CX_SoundStream which wraps RtAudio (https://www.music.mcgill.ca/~gary/rtaudio/). If you are using ofSoundPlayer, depending on your operating system it might eventually use FMOD on Windows or OSx (http://www.fmod.org/; although the openFrameworks maintainers are considering moving away from FMOD) or OpenAL on Linux (http://en.wikipedia.org/wiki/OpenAL). However, you should check that this information is correct.

Pre-experiment Setup
--------------------


Input
-----
The way user input is handled by CX is easily explained by giving the process of receiving a mouse click. Assume that a CX program is running in a window. The user clicks inside of the window. At this point, the operating system (or at least the windowing subsystem of the operating system, but I will choose to conflate them) detects that the click has occured and notes that the location of the click is within the window. The operating system then attempts to tell the program that a mouse click has occured. In order to be notified about input events like mouse click, the program has previously set up a message queue for incoming messages from the operating system. The OS puts the mouse event into the message queue. In order for the program to find out about the message it needs to check to message queue. This is what happens when CX::CX_InputManager::pollEvents() is called: The message queue is checked and all messages in the queue are processed, given timestamps, and routed to the next queue (e.g. the message queue in CX::CX_Mouse that is accessed with CX::CX_Mouse::availableEvents() and CX::CX_Mouse::getNextEvent()). The timestamps are not given by the operating system, so if pollEvents is not called regularly, input events will be received and everything will appear to be working correctly, but the timestamps will be wrong.

Of course, the actual process extends all the way back to the input device itself. The user presses the button and the microcontroller in the input device senses that a button has been pressed. It places this button press event into its outgoing message queue. At the next polling interval (typically 1 ms), the USB host controller on the computer polls the device for messages, discovers that a message is waiting and copies the message to the computer. At some point, the operating system checks to see if the USB host controller has received messages from any devices. It discovers the message and moves the message into the message queue of the program. At each step in which the message moves from one message queue to the next, the data contained in the message likely changes a little. At the start in the mouse, the message might just be "button 1 pressed". At the next step in the USB host controller, the message might be "input device 1 (type is mouse) button 1 pressed". Once the operating system gets the message it might be "mouse button 1 pressed while cursor at (367, 200) relative to clicked window". Eventually, the message gets into the message queue that users of CX work with, in CX::CX_Mouse, for example.

This process sounds very long and complicated, suggesting that it might take a long time to complete, throwing off timing data. That is true: Input timing data collected by CX is not veridical, there are invariably delays, including non-systematic delays. However, there are several steps in the process that no experiment software can get around, so the problems with timing data are not unique to CX. It might be possible to write a custom driver for the mouse or keyboard that allows the software to bypass the operating system's message queue, but it is very difficult to avoid the USB hardware delays. Additionally, the input hardware may not request a polling interval that is slower than the standard 1 ms. The next layer of the problem is that we are really interested in response time to a specific stimulus, but the time at which the stimulus was actually presented may be misreported, so even if the response timestamp had no error whatsoever, when it is compared with the stimulus presentation time, the respones latency would be wrong. Based on this large set of problems with collecting accurate response latency data, it is my firm belief that the only way to accurately measure response latency is with a button box that measures actual stimulus onset time with a light or sound sensor and also measures the time of a button press or other response. If you don't use such a system, the expectation is that you simply allow any error in response latencies to be dealt with statistically. Typically, any systematic error in response times will be subtracted out when conditions are compared with one another. Any random error will simply slightly inflate the estimated variance, but probably not to any meaningful extent.

If you would like to learn more about the internals of how input is handled in CX, you can see how GLFW and openFramworks manage input by examining the source code in the respective repositories.






