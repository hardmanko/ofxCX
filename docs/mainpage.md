Main Page                         {#mainpage}
=========

ofxCX (aka the C++ Experiment System; hereafter referred to as CX) is a "total conversion mod" for openFrameworks (often abbreviated oF) that 
is designed to be used used for creating psychology experiments. OpenFrameworks and CX are based on C++, which
is a very good programming languange for anything requiring a high degree of timing precision. OpenFrameworks 
and CX are both free and open source, distributed under the MIT license.

One of the features that CX has is the ability to run without a substantial installation process.
When a CX program is compiled and linked, the resulting artifact is an executable binary that can be run directly with needing another program to be installed.
The collection of files needed to run a CX program is about 10 MB (possibly more, depending on stimuli). Installing the program just requires unzipping a file.

\section installationInstructions Installation
In order to use CX, you must have openFrameworks installed. Currently only version 0.8.0 of openFrameworks is supported by CX,
which you can download from this page: http://openframeworks.cc/download/older.html The main openFrameworks download page 
(http://openframeworks.cc/download/) has information about how to install openFrameworks for use with some of the more popular 
development environments.

Once you have installed openFrameworks, you can install CX by putting the contents of the CX repository into a subdirectory 
under OFDIR/addons (typically OFDIR/addons/ofxCX), where OFDIR is where you put openFramworks when you installed it. To use CX 
in a project, use the openFrameworks project generator and select ofxCX as an addon (see the instructions for using the 
\ref examplesAndTutorials for information).


\section hardwareRequirements System Requirements

openFrameworks works on a wide variety of hardware and software, some of which are not supported by CX. CX works on computers with certain versions of Windows, Linux, or OSx operating systems. Windows 7 and XP are both supported.

As far as hardware is concerned, the minimum requirements for CX are very low. However, if your video card is too old, you won't be able to use some types of rendering. Having a video card that supports OpenGL version 3.2 at least is good, although older ones will work, potentially with reduced functionality. Also, a 2+ core CPU helps with some things and is generally a good idea for psychology experiments, because one core can be hogged by CX and the operating system can use the other core for other things. Basically, use a computer made after 2010 and you will have no worries whatsoever. However, CX has been found to work with reduced functionality on computers from the mid 90's, so there is that option, although I cannot make any guarantees that it will work on any given computer of that vintage.


\section examplesAndTutorials Examples and Tutorials

There are several examples of how to use CX. The example files can be found in the CX directory 
(see \ref installationInstructions) in subfolders with names beginning with "example-". 
Some of the examples are on a specific topic and others are sample experiments that integrate together different features of CX. 

In order to use the examples and tutorials, do the following:
1. Use the oF project generator (OFDIR/projectGenerator/projectGeneratorSimple.exe) to create a new project that uses the ofxCX addon.
The project generator asks you what to name your project, where to put it (defaults to OFDIR/apps/myApps/), and has the option of selecting
addons. Click the "addons" button and check to box next to ofxCX. If ofxCX does not appear in the list of addons, you probably didn't put the 
ofxCX directory in the right place.
2. Go to the newly-created project directory (that you chose when creating the project in step 1) and go into the src subdirectory.
3. Delete all of the files in the src directory (main.cpp, testApp.h, and testApp.cpp).
4. Copy the example .cpp file into this src directory.
5. If the example has a data folder, copy the contents of that folder into yourProjectDirectory/bin/data. The bin/data folder probably won't 
exist at this point. You can create it.
6. This step depends on your compiler, but you'll need to tell it to use the example source file that you copied in step 4
when it compiles the project (and possibly to specifically not use the files you deleted from the src directory in step 3).
7. Compile and run the project.

Tutorials:
-----------------------
+ soundBuffer - Tutorial covering a number of things that you can do with CX_SoundObjects, including loading sound 
files, combining sounds, and playing them.
+ modularSynth - This tutorial demonstrates a number of ways to generate auditory stimuli using the synthesizer modules in the CX::Synth namespace.
+ dataFrame - Tutorial covering use of CX_DataFrame, which is a container for storing data of various types that is collected in an experiment.
+ logging - Tutoral explaining how the error logging system of CX works and how you can use it in your experiments.
+ animation - A simple example of a simple way to draw moving things in CX. Also includes some mouse input handling: cursor movement, clicks, and scroll wheel activity.

Experiments:
------------------------
+ basicChangeDetection - A very straightforward change-detection task demonstrating some of the features of CX 
like presentation of time-locked stimuli, keyboard response collection, and use of the CX_RandomNumberGenerator.
+ advancedChangeDetection - This example expands on basicChangeDetection, including CX_DataFrame and CX_TrialController 
in order to simplify the experiment.
+ nBack - Demonstrates advanced use of CX_SlidePresenter in the implementation of an N-Back task.

Misc.:
-----------------------
+ helloWorld - A very basic getting started program.
+ renderingTest - Includes several examples of how to draw stuff using ofFbo (a kind 
of offscreen buffer), ofImage (for opening image files: .png, .jpg, etc.), a variety of basic oF drawing functions 
(ofCircle, ofRect, ofTriangle, etc.), and a number of CX drawing functions from the CX::Draw namespace that supplement
openFramework's drawing capabilities.

\section yourFirstExperiment Creating a Blank Experiment

To create your first experiment, follow the steps for using the examples in \ref examplesAndTutorials up to and including step 3. Then create an empty .cpp file in the source directory of the project folder you made. In the file, you will need to include CX_EntryPoint.h and define runExperiment, like in the example below:
\code{.cpp}
#include "CX_EntryPoint.h"

void runExperiment (void) {
	//Do everything you need to do for your experiment
}
\endcode
That's all you need to do to get started. You should look at the \ref examplesAndTutorials in order to learn more about how CX works. You should start with the helloWorld example.


Topics
===========

The best way to get an overview of how CX works is to look at the \ref examplesAndTutorials.

+ To learn about presenting visual stimuli, go to the \ref video page or see the renderingTest or animation examples or the N-Back or change detection example experiments.
+ To learn about playing, recording, and generating sounds, go to the \ref sound page or see the soundObject or modularSynth examples.
+ To learn how to store and output experiment data, see the \ref dataManagement page or see the dataFrame example.
+ To learn about random number generation, see the \ref randomNumberGeneration page.
+ To learn about how CX logs errors and other runtime information, see the \ref errorLogging page.

You can look at the \ref modulesPage page to see the other modules that CX has.

\page modulesPage Modules






