Getting Started {#gettingStarted_page}
===============

In brief, you need a few things to use CX:
+ A reasonably modern computer with Windows, Linux, or OSx.
+ [openFrameworks](http://www.openframeworks.cc), which CX relies on.
+ A C++ compiler/IDE to compile openFrameworks, CX, and your code.
+ The CX code, which you can get from the github repository (https://github.com/hardmanko/ofxCX).

The sections below go into a more detail about each of these things.

System Requirements
-------------------

The short version: Use a reasonably modern computer (made around 2010 or later) with Windows, Linux, or OSx.

The long(er) version:

Although openFrameworks works on a wide variety of hardware and software, CX does not support all of it. For example, CX does not support iPhones, although openFrameworks does.
Windows is the best-supported OS as it is the OS that I use. With Linux, you get what you paid for, but I have gotten CX working well on Linux. OSx is ok, but I haven't used CX very much on that OS. Also, OSx is only an option if you use the latest version of openFrameworks (more below).

As far as hardware is concerned, the minimum requirements for openFrameworks and CX are low. However, if your video card is too old, you won't be able to use some types of graphical rendering. 
Having a video card that supports OpenGL version 3.2 at least is good, although older ones will work, potentially with reduced functionality. 
Also, a 2+ core CPU is generally a good idea for psychology experiments, because one core can be hogged by CX while the operating system can use the other core for other things. Basically, use a computer made after 2010 and you will have no worries most of the time. However, CX has been found to work (with a lot of effect) on computers from the mid 90's, so there is that option, although I cannot make any guarantees that it will work on any given computer of that vintage.

Getting openFrameworks
-------------------------

In order to use CX, you must have openFrameworks installed. You should use version openFrameworks version 0.9.3 or newer, if possible. The oldest version of openFrameworks that is supported is 0.8.4, for now, but there is no reason to avoid using newer versions of openFrameworks. I would recommend oF 0.9.3.

The latest version of openFrameworks can be downloaded from [here](http://openframeworks.cc/download/) and older versions from [here](http://openframeworks.cc/download/older.html). 
The main openFrameworks download page (http://openframeworks.cc/download/) has information about how to install openFrameworks, depending on what development environment you are using.


Compiler/IDE
------------

You will need a C++ compiler/IDE with support for C++11, because CX uses C++11 features extensively. The openFrameworks [download page](http://openframeworks.cc/download/) lists the officially supported IDEs for the different platforms. You can probably make openFrameworks work with other compilers, but this is not recommended for beginners. As far as I know, all compilers/IDEs that support openFrameworks AND have c++11 support will work with CX.

For Windows, I recommend Visual Studio, which is well-supported by openFrameworks. 
+ If using openFrameworks 0.9.0 or newer, I recommend [Visual Studio 2015 Community](https://www.visualstudio.com/en-us/products/visual-studio-community-vs.aspx). It's free.
+ If using openFrameworks 0.8.4, I recommend Visual Studio 2012. The Professional version of it costs money (unless you are a student, in which case it is free through Dreamspark: https://www.dreamspark.com/). If you don't want to buy Visual Studio just to try CX, you can use Visual Studio 2012 Express (http://www.microsoft.com/en-us/download/details.aspx?id=34673), which is free but does not have all of the functionality of the full version of Visual Studio (but it will work).


For Linux, it depends on the version of openFrameworks you're using:
+ For openFrameworks 0.9.0 or newer, I recommend Qt Creator.
+ For openFrameworks 0.8.4, I recommend Code::Blocks.
+ For either, you can use makefilesw ith gcc and avoid an IDE altogether, if you so desire.

For OSx, only openFrameworks 0.9.0 or newer is supported by CX. You can either use Xcode (free from the app store) or Qt Creator (free under some license restrictions), either of which seem to be pretty good options.


Installing CX
-------------

Once you have installed openFrameworks, you can install CX.
First, download CX from [the releases area of the GitHub repository](https://github.com/hardmanko/ofxCX/releases). Select a release that is appropriate for the version of openFrameworks you chose and click on the "Source code (zip)" link to download it (you don't need to download the manual separately, it will be in the `docs` subdirectory within the zip). 
Once the zip file is downloaded it should contain one folder with a name like "ofxCX-0.2.1". Put this folder into `OFDIR/addons`, where OFDIR is where you put openFrameworks when you installed it. The directory structure should be `OFDIR/addons/ofxCX-0.2.1`. Within the folder `ofxCX` there should be a number of folders (`docs`, examples, `libs`, `src`) plus license and readme files. If what you have matches this, you are now done installing things!


Creating Your First CX Project
------------------------------

1. Use the oF project generator to create a new project that uses the ofxCX addon. The help page for the project generator is [here](http://openframeworks.cc/tutorials/introduction/002_projectGenerator.html).
  + The project generator asks you what to name your project and allows you to change where to put it (defaults to `OFDIR/apps/myApps/myAppName`, where `myAppName` is the name you picked for your app).
  + Once you have selected a name and location for the project, in the addons area, select the version of "ofxCX" that you installed. If "ofxCX" does not appear in the list of addons, you probably didn't put the ofxCX directory in the right place when installing it. 
  + Once ofxCX has been added as an addon, click on the "Generate" button to create the project. There are usually no errors when generating a project.

2. Go to the newly-created project directory (that you chose when creating the project in step 1; typically within `OFDIR/apps/myApps`) and go into the `src` subdirectory.

3. Delete all of the files in the `src` directory (main.cpp, testApp.h, and testApp.cpp). The project generator creates these files for normal openFrameworks apps, but you don't need them for CX apps.

4. Create a new .cpp file in the `src` subdirectory and give it a name, like "MyFirstExperiment.cpp". In the new file, you will need to include CX.h and define a function called `runExperiment`, just like in the example below:
\code{.cpp}
#include "CX.h"

void runExperiment (void) {
	//Do everything you need to do for your experiment
}
\endcode
Including CX.h brings into your program all of the classes and functions from CX and openFrameworks so that you can use them. 
`runExperiment` is the CX version of a `main` function: It is called once, after CX has been set up, and the program closes after `runExperiment` returns.

5. Now you need to tell the compiler that it should compile the whole project, including openFrameworks, CX, and your new .cpp file. This step depends on your exact compiler and operating system, but I have provided information for two common configurations.
  + For Visual Studio (VS), you go to the root directory for your application (up one level from `src`) and open the file with the same name as your project with the `.sln` extension. This should open VS and your project. 
    - On the left side of the VS window, there should be a pane called "Solution Explorer". Within the Solution Explorer, there should be a few items. One will be called "Solution 'APP_NAME' (2 projects)", which contains your project, called APP_NAME, and a project called `openframeworksLib`. You should expand your project until you can see a folder called `src`. It will have the same files as you deleted in step 3 listed there, so get rid of them by highlighting them and pressing the delete key (or right click on them and select "Exclude From Project"). 
    - Now right click on the `src` folder in VS and select "Add" -> "Existing item...". In the file selector that opens, navigate your way to the `src` folder in your project directory and select the .cpp file you made in step 4. You can alternately drag and drop your cpp file from the Explorer window into the `src` folder within VS. 
	- Now press F5, or select "Debug" -> "Start Debugging" from the menu bar at the top of the VS window. This will compile and run your project in debug mode. It will take a long time to compile the first time, because it has to compile all of openFrameworks and all of CX the first time. However, subsequent builds will only need to compile your code and will be much faster.
  + On Linux, if you are using Code::Blocks, you don't need to tell Code::Blocks about the new file you made. The build process simply compiles everything in the `src` directory of your project. 
    - Note that on Linux, you may need to explicitly enable C++11 features of the compiler before compiling. When the openFrameworks project generator creates a new project on Linux, it creates a file called config.make in the root directory of your project. Find the line in config.make that has "#PROJECT_CFLAGS" on it and change that line to "PROJECT_CFLAGS = -std=c++11" (note that the # at the start of the line has been removed). This will enable C++11 features of the compiler. If the line is already there and lists a newer C++ standard (like C++14), you don't need to change anything. 
	- After opening the Code::Blocks workspace file, you click on the Compile and Run button (looks like a yellow gear and a green play symbol) to compile and run the project. 

That's all you need to do to get started with a blank experiment. However, you probably have no idea what to put into `runExperiment` at this point. There are two places to start. The first is to read some of the tutorials in this manual, which include \ref visualStimuli, \ref audioIO, \ref responseInput, and \ref dataFrameTutorial. After reading those tutorials, you will have the basics down. The second is to look at the \ref examples_section, which are complete, runnable pieces of code with comments. The advantage of the tutorials is that they are presented in an easy-to-read style. The advantage of the examples is that you can run them and see their output.

\section examples_section Examples

There are several code examples of how to use CX. The example files can be found in the CX directory (`OF_DIR/addons/ofxCX`) in subfolders with names beginning with "example-". Some of the examples are on a specific topic and others are sample experiments that integrate together different features of CX. You should start with the helloWorld example and go from there.

In order to use the examples, do everything for creating a new CX project (above) up until step 3. Then, instead of creating a new .cpp file in step 4, copy one of the example .cpp files from the example folders into the `src` directory. Then do step 5, telling the compiler about the .cpp file you just copied and compiling the example.

Some of the examples have data files that they need run. For example, the renderingTest example has a picture of some birds that it uses. 
If the example has data, in the example directory there will be a directory called `bin` with a directory under it called `data` containing the necessary files. These should be copied to `PROJECT_NAME/bin/data`. The `bin/data` folder in the project directory might not exist immediately after creating a new project. You can create it if it is not there.

Misc. examples:
-----------------------
+ helloWorld - A very basic getting started program.
+ animation - A simple example of a way to draw moving things in CX without using blocking code. Also includes some mouse input handling: cursor movement, clicks, and scroll wheel activity.
+ renderingTest - Includes several examples of how to draw stuff using ofFbo (a kind of offscreen buffer), ofImage (for opening image files: .png, .jpg, etc.), a variety of basic oF drawing functions (ofCircle, ofRect, ofTriangle, etc.), and a number of CX drawing functions from the CX::Draw namespace that supplement openFramework's drawing capabilities.

Experiments:
------------------------
+ flanker - A Flanker task in which letters are used as the stimuli. This is a good minimal experiment example (the other example experiments are substantially more complex). 
+ changeDetection - A very straightforward working memory change-detection task demonstrating some of the features of CX like presentation of time-locked stimuli, keyboard response collection, and use of the CX_RandomNumberGenerator.
There is also an advanced version of the changeDetection task that shows how to do data storage and output with a CX_DataFrame and how to use a custom coordinate system with visual stimuli so that you don't have to work in pixels.
+ nBack - Demonstrates advanced use of CX_SlidePresenter in the implementation of an N-Back task. An advanced version of this example contrasts two methods of rendering stimuli with a CX_SlidePresenter, demonstrating the advantages of each.

Specific topics:
-----------------------
+ dataFrame - Tutorial covering use of CX_DataFrame, which is a container for storing data of various types that is collected in an experiment.
+ logging - Tutoral explaining how the error logging system of CX works and how you can use it in your experiments.
+ soundBuffer - Tutorial covering a number of things that you can do with CX_SoundBuffers, including loading sound files, combining sounds, and playing them.
+ modularSynth - This tutorial demonstrates a number of ways to generate auditory stimuli using the synthesizer modules in the CX::Synth namespace.

