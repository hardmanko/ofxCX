Main Page                         {#mainpage}
=========

ofxCX (hereafter referred to as CX) is a "total conversion mod" for openFrameworks (often abbreviated oF) that 
is designed to be used used for creating psychology experiments. OpenFrameworks and CX are based on C++, which
is a very good option for anything requiring a high degree of timing precision. OpenFrameworks and CX are both
free and open source, distributed under the MIT license.

\section installationInstructions Installation
In order to use CX, you must have openFrameworks installed. See http://openframeworks.cc/download/ to download openFrameworks.
On that page, you can algo find installation guides for some of the most popular development environments.
Currently only version 0.8.0 of openFrameworks is supported by CX.

Once you have installed openFrameworks, you can install CX by putting the contents of the CX repository into a subdirectory 
under OFDIR/addons (typically OFDIR/addons/ofxCX), where OFDIR
is where you put openFramworks when you installed it.

To use CX in a project, use the openFrameworks project generator and select ofxCX as an addon. The project generator can
be found in OFDIR/projectGenerator.

In order to use the \ref examplesAndTutorials, do the following:
1. Use the oF project generator (OFDIR/projectGenerator/projectGeneratorSimple.exe) to create a new project that uses the ofxCX addon.
The project generator asks you what to name your project, where to put it (defaults to OFDIR/apps/myApps/), and has the option of selecting
addons. Click the "addons" button and check to box next to ofxCX. If ofxCX does not appear in the list of addons, you probably didn't put the 
ofxCX directory in the right place.
2. Go to the newly-created project directory (that you chose when creating the project in step 1) and go into the src subdirectory.
3. Delete all of the files in the src directory (main.cpp, testApp.h, and testApp.cpp).
4a. Copy the example .cpp file into this src directory.
4b. If the example has a data folder, copy the contents of that folder into yourProjectDirectory/bin/data. bin/data folders probably won't 
exist at this point. You can create them.
5. This step depends on your compiler, but you'll need to tell it to use the example source file that you copied in step 4a
when it compiles the project (and possibly to specifically not use the files you deleted from the src directory in step 3).
6. Compile and run the project.


\section yourFirstExperiment Getting Started

To create your first experiment, follow the steps for using the examples in \ref installationInstructions up to and including step 3. Then create a blank .cpp file in the source directory of the project folder you made. In the file, you will need to include CX_EntryPoint.h and define runExperiment, like in the example below:
\code{.cpp}
#include "CX_EntryPoint.h"

void runExperiment (void) {
	//Do your thing
}
\endcode
That's all you need to do to get started. You should look at the \ref examplesAndTutorials in order to learn more about how CX works. You should start with the helloWorld example.

\section examplesAndTutorials Examples and Tutorials

There are several examples that serve as tutorials for CX. Some of the examples are on a specific topic and others 
are sample experiments that integrate together different features of CX. The example files can be found in the CX
directory (see \ref installationInstructions) in subfolders with names beginning with "example-".

Tutorials:
-----------------------
+ soundBuffer - Tutorial covering a number of things that you can do with CX_SoundObjects, including loading sound 
files, combining sounds, and playing them.
+ modularSynth - This tutorial demonstrates a number of ways to generate auditory stimuli using the synthesizer modules in the CX::Synth namespace.
+ dataFrame - Tutorial covering use of CX_DataFrame, which is a container for storing data that is collected in an experiment.
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
+ renderingTest - Includes several examples of how to draw stuff using ofPath (arbitrary lines), ofTexture (a kind 
of pixel buffer), ofImage (for opening image files: .png, .jpg, etc.), a variety of basic oF drawing functions 
(ofCircle, ofRect, ofTriangle, etc.), and a number of CX drawing functions from the CX::Draw namespace.

Topics
===========

The best way to get an overview of how CX works is to look at the \ref examplesAndTutorials.

+ To learn about presenting visual stimuli, go to the \ref video page or see the renderingTest or animation examples or the N-Back or change detection example experiments.
+ To learn about playing, recording, and generating sounds, go to the \ref sound page or see the soundObject or modularSynth examples.
+ To learn how to store and output experiment data, see the \ref dataManagement page or see the dataFrame example.
+ To learn about random number generation, see the \ref randomNumberGeneration page.
+ To learn about how CX logs errors and other runtime information, see the \ref errorLogging page.
+ To learn about the 

You can look at the \ref modulesPage page to see the other modules that CX has.

\page modulesPage Modules






