Main Page                         {#mainpage}
=========

ofxCX (hereafter referred to as CX) is a "total conversion mod" for openFrameworks (often abbreviated oF) that 
is designed to be used used for creating psychology experiments.

The most well-organized way to access the documentation is go to the \ref modulesPage page. The best way to get
an overview of how CX works is to look at the \ref examplesAndTutorials.

To learn about presenting visual stimuli, go to the \ref video page.

To learn about auditory stimuli, go to the \ref sound page.

To learn how to store and output experiment data, see the \ref dataManagement page.

To learn about random number generation, see the \ref randomNumberGeneration page.

To learn about how CX logs errors and other runtime information, see the \ref errorLogging page.


\section installationInstructions Installation
In order to use CX, you must have openFrameworks installed. See http://openframeworks.cc/download/ to download openFrameworks.
Currently only version 0.8.0 of openFrameworks is supported by CX.

Once you have installed openFrameworks, you can install CX by putting the contents of the CX repository into a subdirectory 
under openFrameworksDirectory/addons (typically openFrameworksDirectory/addons/ofxCX), where openFrameworksDirectory
is where you put openFramworks when you installed it.

To use CX in a project, use the openFrameworks project generator and select ofxCX as an addon. The project generator can
be found in openFrameworksDirectory/projectGenerator.

In order to use the examples, do the following:
1. Use the oF project generator (in openFrameworksDirectory/projectGenerator) to create a new project that uses the ofxCX addon.
2. Go to the newly-created project directory (that you chose when creating the project in step 1) and go into the src subdirectory.
3. Delete all of the files in the src directory (main.cpp, testApp.h, and testApp.cpp).
4a. Copy the example .cpp file into this directory.
4b. If the example has a data folder, copy the contents of that folder into yourProjectDirectory/bin/data. bin/data folders probably won't 
exist at this point. You can create them.
5. This step depends on your compiler, but you'll need to tell it to use the example source file that you copied in step 4a
when it compiles the project (and possibly to specifically not use the files you deleted from the src directory in step 3).
6. Compile and run the project.

\section examplesAndTutorials Examples and Tutorials
There are several examples that serve as tutorials for CX. Some of the examples are on a specific topic and others 
are sample experiments that integrate together different features of CX. The example files can be found in the CX
directory (see \ref installationInstructions) in subfolders with names beginning with "example-".

Tutorials:
-----------------------
+ soundObject - Tutorial covering a number of things that you can do with CX_SoundObjects, including loading sound 
files, combining sounds, and playing them.
+ dataFrame - Tutorial covering use of CX_DataFrame, which is a container for storing data that is collected in an experiment.
+ logging - Tutoral explaining how the error logging system of CX works and how you can use it in your experiments.

Experiments:
------------------------
- basicChangeDetection - A very straightforward change-detection task demonstrating some of the features of CX 
like presentation of time-locked stimuli, keyboard response collection, and use of the CX_RandomNumberGenerator.
+ advancedChangeDetection - This example expands on basicChangeDetection, including CX_DataFrame and CX_TrialController 
in order to simplify the experiment.
+ nBack - Demonstrates advanced use of CX_SlidePresenter in the implementation of an N-Back task.

Misc.:
-----------------------
+ helloWorld - A very basic getting started program.
+ animation - A simple example of the most simple way to draw moving things in CX. Also includes some mouse stuff: 
cursor movement, clicks, and scroll wheel activity.
+ renderingTest - Includes several examples of how to draw stuff using ofPath (arbitrary lines), ofTexture (a kind 
of pixel buffer), ofImage (for opening image files: .png, .jpg, etc.), a variety of basic oF drawing functions 
(ofCircle, ofRect, ofTriangle, etc.), and a number of CX drawing functions from the CX::Draw namespace.

\page modulesPage Modules






