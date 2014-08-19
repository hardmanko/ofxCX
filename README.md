ofxCX
=====================================

Introduction
------------
ofxCX (hereafter referred to as CX) is a "total conversion mod" for [openFrameworks](http://www.openframeworks.cc) (often abbreviated oF) that 
is designed to be used used for creating psychology experiments. OpenFrameworks and CX are based on C++, which
is a very good language for anything requiring a high degree of timing precision. OpenFrameworks and CX are both
free and open source.

The documentation for CX can be found in the docs subfolder and in the code files. A generally fairly up-to-date pdf version of the documentation is contained in docs/CX Manual.pdf (or try [this link](https://sites.google.com/site/kylehardmancom/files/CX%20Manual.pdf?attredirects=0&d=1)). You can generate 
other formats of the documentation by using Doxygen with the Doxyfile given in the docs subfolder.

License
-------
This addon is distributed under the MIT license (see license.md).

Installation
------------
Drop the contents of this repository into a subdirectory directory under %openFrameworksDirectory%/addons 
(typically addons/ofxCX). See the manual for more installation information, like how to get openFrameworks 
and how to use the examples for CX.

Compatibility
------------
CX only works with oF version 0.8.0, so make sure you are using that version and not the latest version. It has only been tested on Windows, but may work completely on OSx. Some functionality is missing on Linux.

As far as compilers/IDEs are concerned, CX has been tested under Visual Studio 2012/2013 with the VS2012 compiler and Code::Blocks for Windows using GCC.

Known issues
------------
This is alpha/beta software, don't expect it to be bug-free. Please report any issues you have in the issue tracker.

Loading sound files with CX_SoundBuffer uses FMOD, which is not available on Linux. Other ways of playing sound are supported.

Version history
------------
There has not yet been a versioned release of CX.
