ofxCX
=====================================

Introduction
------------
ofxCX (hereafter referred to as CX) is a "total conversion mod" for [openFrameworks](http://www.openframeworks.cc) (often abbreviated oF) that 
is designed to be used used for creating psychology experiments. OpenFrameworks and CX are based on C++, which
is a very good language for anything requiring a high degree of timing precision, such as psychology experiments. OpenFrameworks and CX are both
free and open source. See [this page](http://www.kylehardman.com/psychology/c-experiment-software) for a summary of the features of CX.

The documentation for CX can be found in the `docs` subfolder and in the code files. A fairly up-to-date pdf version of the documentation is contained in `docs/CX_Manual.pdf` 
(or try [this link](https://github.com/hardmanko/ofxCX/releases/download/v0.1.0/CX_Manual.pdf) if you don't want to download the whole repo). 
You can generate other formats of the documentation by using Doxygen with the Doxyfile in the `docs` subfolder.

License
-------
This addon is distributed under the MIT license (see `license.md`).

Installation
------------
Pick a release to download or just download the current version of the repository.
Put the contents of whatever you downloaded into a subdirectory directory under `%openFrameworksDirectory%/addons` (typically `%openFrameworksDirectory%/addons/ofxCX`). 
See the manual for more installation information, like how to get openFrameworks and how to use the examples for CX.

Compatibility
------------
CX works with oF versions 0.8.4 and 0.8.0, so make sure you are using one of those versions (intervening versions should probably work as well, but they have not been tested).

CX is compatible with Windows and Linux and is tested and working on both platforms. As far as compilers/IDEs are concerned, CX has been tested under Visual Studio 2012 with the Microsoft Visual C++ compiler and Code::Blocks for Windows using GCC. On Linux, CX has been tested with Code::Blocks using GCC.

CX is not compatible with OSx because CX uses C++11 features extensively, but openFrameworks does not support C++11 on OSx (openFrameworks does not officially support C++11 on Linux or Windows either, yet there are no problems with either of those OSes, so I blame Apple, not the openFrameworks devs).

Known issues
------------
This is beta software, don't expect it to be bug-free. Please report any issues you have in the issue tracker.

Version history
------------
v0.1.0 - Intial release.
