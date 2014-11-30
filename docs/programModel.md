Program Model {#programModel}
=============

Internals/Attributions
----------------------
CX is not a monolithic entity. It is based on a huge amount of open source software written by many different authors over the years. Clearly, CX is based on openFrameworks, but openFramworks itself is based on many different libraries. Window handling, which involves creating a window that can be rendered to and receiving user input events from the operating system, is managed by GLFW (http://www.glfw.org/). The actual rendering is visual stimuli is done using OpenGL (http://www.opengl.org/), which is wrapped by several openFrameworks abstractions (e.g. ofGLProgrammableRenderer at a lower level, e.g. ofPath at the level at which a typical user would use).

Audio is processed in different ways depending on the type of audio player used. CX_SoundBufferPlayer and CX_SoundBufferRecorder wrap CX_SoundStream which in turn wraps RtAudio (https://www.music.mcgill.ca/~gary/rtaudio/). If you are using ofSoundPlayer, depending on your operating system it might eventually use FMOD on Windows or OSx (http://www.fmod.org/; although the openFrameworks maintainers are considering moving away from FMOD) or OpenAL on Linux (http://en.wikipedia.org/wiki/OpenAL).

There are other libraries that are a part of openFrameworks that I am not as familiar with, including Poco (http://pocoproject.org/), which provides a variety of very useful utility functions and networking, FreeType (http://www.freetype.org/) which does font rendering, and many others.

Additionally, CX uses the colorspace package by Pascal Getreuer (http://www.getreuer.info/home/colorspace).

CX would not have been possible without the existence of these high-quality open-source projects.


Overriding openFrameworks
-------------------------
Although CX is technically an addon to openFrameworks, there are a number of ways in which CX hijacks normal oF functionality in order to work better. As such, you cannot assume that all oF functionality is available to you.

Generally, drawing visual stimuli using oF classes and functions is fully supported. See the renderingTest example to see a pletora of ways to put things on the screen.

Audio output using ofSoundPlayer is supported, although no timing guarantees are made. Prefer CX::CX_SoundBufferPlayer.

The input events (e.g. ofEvents().mousePressed) technically work, but with two serious limitations: 1) The events only fire when CX_InputManager::pollEvents() is called (which internally calls glfwPollEvents() to actually kick off the events firing) and 2) The standard oF events do not have timestamps, which limits their usefulness.

The following functions' behavior is superseded by functionality provided by CX_Display (see also CX::Instances::Display): 
ofGetFrameNum() is replaced by CX_Display::getFrameNumber()
ofGetLastFrameTime() is replaced by CX_Display::getLastSwapTime()

The following functions do nothing: ofGetFrameRate(), ofSetFrameRate(), ofGetTargetFrameRate()

A variety of behaviors related to ofBaseApp do not function because CX is not based on a class derived from ofBaseApp nor does it use ofRunApp() to begin the program. For example, a standard oF app class should have steup(), update(), and draw() functions that will be called by oF during program execution. CX has a different model that does not force object-orientation on design of the user's code.


Program Flow
------------
One of the foundational aspects of CX is the design of the overall program flow, which includes things such as how responses are collected, how stimuli are drawn to the screen, and other similar concepts. The best way to learn about program flow is to look at the examples. The examples cover most of the critical topics and introduce the major components of CX.

The most important thing to understand is that in CX, nothing happens that your code does not explicitly ask for, with the exception of a small amount of setup, which is discussed below . For example, CX does not magically collect and timestamp user responses for you. Your code must poll for user input in order to get timestamps for input events. This is explained more in the \ref responseInput section. In CX, there is no code running in the background that makes everything work out for your experiment: you have to design your experiment in such a way that you are covering all of your bases. That said, CX is designed to make doing that as easy and painless as possible, while still giving you as much control over your experiment as possible.

There is very little that CX does without you asking for it. The one major exception is pre-experiment setup, in which a number of basic operations are performed in order to set up a platform on which the rest of the experiment can run. The most significant step is to open a window and set up the OpenGL rendering environment. If you don't like the default rendering environment, use CX::relaunchWindow() to make a new window with different settings. The main pseudorandom number generator (CX::Instances::RNG) is seeded. The logging system is prepared for use. The main clock (CX::Instances::Clock) is prepared for use.



