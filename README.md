# LiveCode Community Edition

![LiveCode Community Logo](http://livecode.com/wp-content/uploads/2015/02/livecode-logo.png)

Copyright © 2003-2019 LiveCode Ltd., Edinburgh, UK

## Introduction

The LiveCode Community open source platform provides a way to build applications for mobile, desktop and server platforms.

The visual workflow allows the user to develop apps "live", using a powerful and uniquely-accessible language syntax.

[LiveCode Ltd.](http://livecode.com/), based in Edinburgh, UK, coordinates development of LiveCode and has run the open source project since LiveCode Community was first released in April 2013.

You can download pre-compiled versions of the LiveCode Community development environment from the [LiveCode download centre](http://downloads.livecode.com/livecode/).

## Overview

### Subproject directories

This repository contains a number of subprojects, each of which has its own subdirectory.  They can be divided into three main categories.

1. Main system:

  * `engine/` — The main LiveCode engine.  This directory produces the IDE, "standalone", "installer" and "server" engines

2. Non-third-party libraries:

  * `libcore/` — A static library that provides various basic functions and types, and is used by many of the other subprojects

  * `libexternal/` and `libexternalv1` — Static libraries that support the LiveCode "external" interface, which allows the engine to load plugins

3. Externals (libraries that can be dynamically loaded into the engine at runtime):

  * `revdb/` — Database access external, and drivers for various backend database systems

  * `revmobile/` — The iOS support external (which can only be built on Mac) and the Android support external (available on all desktop platforms)

  * `revpdfprinter/` — Print-to-PDF functionality

  * `revspeech/` — Text-to-speech support

  * `revvideograbber/` — Video capture (Windows only)

  * `revxml/` — XML parsing and generation

  * `revzip/` - Zip archive management

### Engine flavours

The engine — which loads, saves, manages and runs LiveCode stack files — can be built in several different specialized modes, which are adapted for various specific purposes.  They are exposed as separate targets in the build system.

1. **IDE engine** (`development` target)— Used to run the IDE.  It contains extra support for things like syntax handling and building LiveCode "standalone" programs.

2. **Installer engine** (`installer` target) — Used to create the LiveCode installer.  It contains extra support for things like handling zip archives and comparing binary files.

3. **Server engine** (`server` target) — This is the engine used in a server context, when no graphical user interface is needed.  It contains server-specific functions such as CGI support.  It also has a much fewer system library dependencies (and requires only non-desktop APIs where possible).

4. **Standalone engine** (`standalone` target) — The engine that is embedded in "standalone apps" created with LiveCode.

## Compiling LiveCode

LiveCode uses the [gyp (Generate Your Projects)](https://chromium.googlesource.com/external/gyp.git) tool to generate platform-specific project files.  It can generate `xcodeproj` files for Xcode on Mac, `vcproj` files for Microsoft Visual Studio, and makefiles for compiling on Linux.

### Quick start

**Note**: You can only compile LiveCode from a clone of the
[LiveCode git repository](https://github.com/livecode/livecode) on
GitHub.  See also the GitHub documentation on
[cloning a repository](https://help.github.com/articles/cloning-a-repository/).

On Linux or Mac, you can quickly build LiveCode by installing basic development tools, and then running `make all`.

### Detailed instructions

Please see the following table, which shows which target platforms are supported by which host platforms.  The documentation for compiling for each target platform is linked.

| Target platform                                            | Host platforms    |
| ---------------------------------------------------------- | ----------------- |
| [mac, ios](docs/development/build-mac.md)                  | mac               |
| [win](docs/development/build-win.md)                       | win, linux (Wine) |
| [linux](docs/development/build-linux.md)                   | linux             |
| [android](docs/development/build-android.md)               | mac, linux        |
| [emscripten (html5)](docs/development/build-emscripten.md) | linux             |

## Getting help

There are several ways to get help with installing and using LiveCode:

* If you have a LiveCode Commercial subscription, you may [contact LiveCode Support](mailto:support@livecode.com).

* Ask a question on the ["use-livecode" mailing list](http://lists.runrev.com/mailman/listinfo/use-livecode), or search the the mailing list archives for previous answers to similar questions

* Visit the [LiveCode open source forums](http://forums.livecode.com/viewforum.php?f=65).  In particular, you may wish to post your question in the [Engine Contributors](http://forums.livecode.com/viewforum.php?f=66) forum.

* If you have discovered a bug, have a feature request, or have written a patch to improve LiveCode, please create an ticket in the [LiveCode issue tracking system](http://quality.livecode.com/).

## Contributing to LiveCode

For information on modifying LiveCode and submitting contributions to the LiveCode Community project, please see the [CONTRIBUTING](CONTRIBUTING.md) file.

## License

LiveCode Community is freely distributable under the GNU Public License (GPL), with some special exceptions.  For more information, please see the [LICENSE](LICENSE) file in this repository.

The LiveCode Community engine, libraries, and associated files are, unless otherwise noted:

Copyright © 2003-2019 LiveCode Ltd.
