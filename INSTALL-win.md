# Compiling LiveCode for Windows

![LiveCode Community Logo](http://livecode.com/wp-content/uploads/2015/02/livecode-logo.png)

Copyright © 2015 LiveCode Ltd., Edinburgh, UK

## Dependencies

The Windows build scripts currently don't have any ability to auto-discover tools, so you need to **install all of the build dependencies to their default locations**.

### git

You will need to install [git for Windows](https://git-scm.com/download/win) in order to obtain the LiveCode source code from GitHub.

### Microsoft Visual Studio

You will need to download & install [Microsoft Visual Studio 2010 Express](http://download.microsoft.com/download/1/E/5/1E5F1C0A-0D5B-426A-A603-1798B951DDAE/VS2010Express1.iso) (or Professional, if available).

In addition, you should install:

* [Microsoft Speech SDK 5.1](https://www.microsoft.com/en-gb/download/details.aspx?id=10121)
* Microsoft Speech SDK 4.0 - this can be harder to find, but [this link](ftp://ftp.boulder.ibm.com/software/viavoicesdk/sapi4sdk.exe) may work

### Cygwin

The build currently requires the use of some tools from the Cygwin distribution of GNU and other open source tools.

You need to [install Cygwin](https://cygwin.com/install.html), along with the following additional packages:

* make
* bash
* bison
* flex
* curl
* zip
* unzip

### Other tools

The build process also requires:

* [ActiveState Perl](https://www.activestate.com/activeperl/downloads) Community Edition
* [Python 2.7](https://www.python.org/) (Python 3 isn't supported)
* [QuickTime 7.3 SDK for Windows](https://developer.apple.com/downloads)

## Configuring LiveCode

Once you have checked out the source code from git, you can run:

````
cmd /C configure.bat
````

(Or just run `configure.bat` by double-clicking on it from Windows Explorer)

This will generate a set of Visual Studio project files in the `build-win-x86/livecode` directory.

## Compiling LiveCode

To compile LiveCode, you can either open the `build-win-x86/livecode/livecode.sln` solution file in Visual Studio, or you can run:

````
cd build-win-x86
cmd /C ..\make.cmd
````

Note that if you are using Visual Studio 2010 Express you won't be able to compile the revbrowser extension.
