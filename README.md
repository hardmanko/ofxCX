ofxCX
=====================================

Introduction
------------
ofxCX (hereafter referred to as CX) is a "total conversion mod" for openFrameworks that is designed to be used used for creating psychology experiments.

There are several examples that serve as tutorials for CX. Some of the examples are on a specific topic and others are sample experiments that integrate together different features of CX.

Topics:
soundObject - Tutorial covering a number of things that you can do with CX_SoundObjects, including loading files, combining sounds, and playing them.
dataFrame - Tutorial covering use of CX_DataFrame, which is a container for storing data that is collected in an experiment.
logging - Tutoral explaining how the error logging system of CX works.

Experiments:
basicChangeDetection - A very straightforward change-detection task demonstrating some of the features of CX like presentation of time-locked stimuli, response collection, and use of the CX_RandomNumberGenerator.
advancedChangeDetection - This example expands on basicChangeDetection, including CX_DataFrame and CX_TrialController in order to simplify the experiment.
nBack - Demonstrates advanced use of CX_SlidePresenter in the implementation of an N-Back task.

Misc.:
helloWorld - A very basic getting started program.
animation - A simple example of the most simple way to draw moving things in CX.

Licence
-------
This addon is distributed under the MIT license (see license.md).

Installation
------------
Drop the contents of this repository into a subdirectory directory under %openFrameworksDirectory%/addons (typically addons/ofxCX).

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


