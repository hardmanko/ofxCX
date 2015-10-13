Deploying an Experiment {#experimentDeployment}
=======================

Most of this manual covers how to program an experiment in CX. This section covers how to take an experiment that works in debug mode on a development computer and distribute that experiment to computers on which the experiment will be run.

For all operating systems, one of the most important things to do is to recompile the experiment in a non-debug mode (often called "release" mode). When a program is compiled in debug mode, the compiler adds a large amount of code to the program so that it is easier to debug. One example of something that might be added to a debug version is bounds checking on containers. Assume that you have a vector with 10 elements and you try to access the 12th element using operator[] (the standard way to get data out). This will result in erroneous program behavior regardless of the way in which the program is compiled. However, in debug mode, extra bounds-checking code has been added and when the out-of-bounds access is detected, the debugger triggers a breakpoint and complains that your code is misbehaving. In release mode, no bounds checking is performed, so that what would happen is that whatever is in memory just past the end of the vector would get accessed. This could result in the program crashing right away, or it could result in gradual memory corruption that eventually results in a crash. Thus, release mode programs are much harder to debug than debug mode programs. However, release mode programs do not have all of the extra debugging code added to them and, as a result, run much more quickly than debug mode programs. This speed boost is why you should compile your programs in release mode when preparing to use them to collect data. In addition, there is a class of strange timing-related bugs that only happen in debug mode.

Because the deployment process differs depending on your operating system, each OS will be covered individually. For the big picture, the steps are
1. Compile the experiment in release mode.
2. Collect any additional dependencies (depends on operating system).
3. Copy the contents of `PROJECT_DIRECTORY/bin` to the target computer.


Windows
-------

The result of compilation of a CX experiment is an executable file. This file is `OFDIR/apps/myApps/PROJECT_NAME/bin/PROJECT_NAME.exe`, where `OFDIR` is the root directory of your openFrameworks installation and PROJECT_NAME is the name of your project. If you have not built a release version of your program yet, you won't have this file. If you have built a debug version of your program, you should have a file called `PROJECT_NAME_debug.exe` in this directory. Also in the `bin` directory, there are a number of files with the same name as the project, but with different file extensions than `.exe`. You do not need these files and may delete them.  

Although you have an executable file, it is not without dependencies. In addition to the .exe, there are several .dll files in the `bin` directory. Many of these dlls are needed in order for your program to be able to run and which ones are needed depends on specifics about what your code does. If you would like to test whether you need any of these dlls, you can rename them by, for example, added an "_" to the front of their name and running the executable file. If the experiment opens without error, you do not need the dll that you renamed. A more general way to learn about the dependencies of a Windows executable is to use the Dependency Walker (http://www.dependencywalker.com/).

In addition to the dlls that are in the `bin` directory, you will need some additional dlls that are used by Visual Studio. For Visual Studio 2012, their names are msvcp110.dll and msvcr110.dll (the 110 is because, internally, VS 2012 is version 11.0 of Visual Studio). The first file (distinguished by the "p" in the name) is the C++ standard library implementation and the second file (with "r") is the C runtime library. If the default installation location for Visual Studio was used, these files can be found in `C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\redist\x86\Microsoft.VC110.CRT\`. Copy them from that location to the `bin` directory in your project to make them available to your executable file regardless of where you copy the `bin` directory to. For more information, you can see this page: https://msdn.microsoft.com/en-us/library/ms235299%28v=vs.110%29.aspx. 

If for whatever reason (perhaps testing), you need to deploy a program that was compiled in debug mode, you will need the debug versions of msvcp110.dll and msvcr110.dll. The debug versions of these files are msvcp110d.dll and msvcr110d.dll (note the "d" at the end of the names) and can be found in `C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\redist\Debug_NonRedist\x86\Microsoft.VC110.DebugCRT\`. Note that part of that path is `Debug_NonRedist`, indicating that these dlls are not supposed to be redistributed under the Visual Studio license terms. I am not a lawyer, but I suspect that if you deploy the debug dlls along with your project to computers within your lab (or even multiple labs within a single institution) that you should be fairly safe from Microsoft coming down on you, especially if you have a good reason (like testing). However, I would strongly recommend against including the debug dlls in anything that is released publicly. In any case, you should only deploy programs that have been compiled in release mode.

Once you have added the two MSCV dlls to the `bin` directory, you should be able to just copy all of the contents of `bin` to another computer and have the experiment run. This is why the `data` directory is within `bin`: So that the `bin` directory is a self-contained copy of the experiment, including all data files.

In Visual Studio, switching from debug mode to release mode is easy. In the default view, in the toolbars at the top of the screen, there should be a dropdown menu that has the word "Debug" in it as the currently selected option. Click on the dropdown and select "Release". The next time you compile the project, it will be compiled in release mode. It may take a long time to compile the first time, as everything needs to be recompiled.

### Windows XP

If you are deploying to a computer running Windows XP, there is an additional step. You must compile your project and the openFrameworks project to be compatible with Windows XP, which it is not done by default (Windows XP is obsolete). Assume that you are in Visual Studio and the Solution Explorer pane is visible. At the top level, there will be a Solution that has the same name as your project. In terms of the hierarchy, directly under the Solution there will be your project and a project titled openframeworksLib. You must complete the following steps for both projects.

Right-click on the project and select Properties. A window titled PROJECT_NAME Property Pages should open. On the left, under Configuration Properties, select General. On the right, there should be a setting called "Platform Toolset". If you click on the selection, a dropdown button should appear and if you click on that, a dropdown menu should appear. Select the option called "Visual Studio 2012 - Windows XP (v110)".

Again, do this for both your project and the openframeworksLib project, then recompile the project. If you compile for Windows XP, it will work on some newer versions of Windows as well, but there may be some versions of Windows that it will not work for.



Linux
-----
In general, the situation is a bit easier on Linux than on Windows. For one thing, many of the program dependencies that come as dlls on Windows get compiled into the executable on Linux. You should not need to dig around to get extra dependencies and should be able to just deploy the contents of the `bin` directory to other Linux computers. Of course, you would probably be well-served to use the same distribution and version of Linux on the development and deployment computers to avoid any hassle about different versions of Linux.



