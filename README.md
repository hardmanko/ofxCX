ofxCX
=====================================

Introduction
------------
ofxCX (hereafter referred to as CX) is a "total conversion mod" for openFrameworks (often abbreviated oF) that is designed to be used used for creating psychology experiments.

There are several examples that serve as tutorials for CX. Some of the examples are on a specific topic and others are sample experiments that integrate together different features of CX.

Topics:
soundObject - Tutorial covering a number of things that you can do with CX_SoundObjects, including loading sound files, combining sounds, and playing them.
dataFrame - Tutorial covering use of CX_DataFrame, which is a container for storing data that is collected in an experiment.
logging - Tutoral explaining how the error logging system of CX works.

Experiments:
basicChangeDetection - A very straightforward change-detection task demonstrating some of the features of CX like presentation of time-locked stimuli, response collection, and use of the CX_RandomNumberGenerator.
advancedChangeDetection - This example expands on basicChangeDetection, including CX_DataFrame and CX_TrialController in order to simplify the experiment.
nBack - Demonstrates advanced use of CX_SlidePresenter in the implementation of an N-Back task.

Misc.:
helloWorld - A very basic getting started program.
animation - A simple example of the most simple way to draw moving things in CX.
renderingTest - Includes several examples of how to draw stuff using ofPath (arbitrary lines), ofTexture (a kind of pixel buffer), ofImage (for opening image files: .png, .jpg, etc.), a variety of basic oF drawing functions (ofCircle, ofRect, ofTriangle, etc.), and a number of CX drawing functions from the CX::Draw namespace.

Licence
-------
This addon is distributed under the MIT license (see license.md).

Installation
------------
Drop the contents of this repository into a subdirectory directory under %openFrameworksDirectory%/addons (typically addons/ofxCX).

In order to use the examples, do the following:
1) Use the oF project generator (in %oF_directory%/projectGenerator) to create a new project that uses the ofxCX addon.
2) Go to the newly-created project directory (that you chose when creating the project in step 1) and go into the src subdirectory. 
3) Delete all of the files in the src directory (main.cpp, testApp.h, and testApp.cpp)
4) Copy the example .cpp file into this directory
4a) If the example has a data folder, copy the contents of that folder into %yourProject%/bin/data. These folders probably won't exist at this point. You can create them.
5) This step depends on your compiler, but you'll need to tell it to use the example source file (.cpp) when it compiles the project (and possibly to specifically not use the files you deleted from the src directory in step 3).
6) Compile and run the project

Dependencies
------------
None

Compatibility
------------
CX only works with oF version 0.8.0. It has only been tested on Windows, but may work completely on OSx. Some functionality is missing on Linux.

Known issues
------------
This is alpha software, don't expect it to be bug-free.

Version history
------------
It make sense to include a version history here (newest releases first), describing new features and changes to the addon. Use [git tags](http://learn.github.com/p/tagging.html) to mark release points in your repo, too!

### Version 0.1 (Date):
Describe relevant changes etc.


