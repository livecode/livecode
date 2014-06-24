LiveCode Source Repository
==========================

This is the source-code repository for LiveCode <http://www.livecode.com>.

Build Overview
--------------
The build system is (currently) very platform-specific in that there are distinct sets of projects (or Makefiles) for each platform.

At the top-level there are folders for each of the non-third-party static libraries, the externals and the engine itself:
* libcore: this is a static library used by several other components that provides various basic functions and types.
* libexternal(v1): these are the static libraries that support the LiveCode external interface.
* engine: this is the main engine component, this folder produces the IDE, Standalone, Installer and Server engines.
* revdb: this contains projects to build the revdb external and associated drivers.
* revmobile: this contains projects to build the iOS support external (for Mac) and the Android support external (for Mac, Windows and Linux).
* revpdfprinter: this contains the project to build the revpdfprinter component (for print to pdf functionality).
* revspeech: this contains the project to build the revspeech external.
* revvideograbber: this contains the project to build the revvideograbber external.
* revxml: this contains the project to build the revxml external.
* revzip: this contains the project to build the revzip external.

The most complex component is the main engine (unsurprisingly!). In particular, there are several variants of the engine that are specialised for particular uses. These are exposed as different targets in each of the projects (on each platform):

* IDE engine: This is the engine that is used to run the IDE. It contains extra functionality (compared to standalones) for things like syntax highlighting and standalone building. On each platform the target has a slightly different name:
  * Mac: LiveCode-Community
  * Linux: development
  * Windows: engine
*	Installer engine: This is the engine that is used to build the installer. It contains extra functionality for things like unzipping and processing binary differences. On each platform the target has a slightly different name:
  * Mac: Installer-Community
  * Linux: installer
  * Windows: installer
*	Server engine: This is the engine used in a server context. It contains server-specific functionality such as CGI support. It also has a much more minimal requirement on system libraries (depending as much as possible on non-Desktop related APIs). On each platform the target has a different name:
  * Mac: Server-Community
  * Linux: server
  * Windows: server
*	Standalone engine: This is the engine that is used to build standalones. Again, on each platform the target has a slightly different name:
  * Mac: Standalone-Community
  * Linux: standalone
  * Windows: standalone
  * iOS: standalone-mobile-community
  * Android: no target exactly, as Android builds are done using a shell script that builds all components together (a mixture of the NDK build system and Java compilation).

The systems used for each platform is as follows:
* Mac: Uses Xcode and xcode projects. The top-level project for this (that builds all components) is stage.xcodeproj which references individual xcodeproj's in each relevant folder. (There are two xcodeproj's in each relevant folder – one for desktop, one for iOS). For example, the main engine project is engine/engine.xcodeproj.
*	Linux: Uses make. There is a top-level Makefile which references Makefiles in each relevant folder. Since some components have several sub-components (usually static libraries), many folders contain multiple Makefiles. The top-level Makefile has rules for each of the individual components, e.g. to build the IDE (development) engine use make development.
*	Windows (Desktop): Uses Visual C++ 2005. There is a top-level solution stage.sln which references all the projects in the sub-folders. Loading this solution into VS allows you to choose which targets to build, or just to build the whole solution.
*	Windows (Server): Uses Visual C++ 2005. There is a top-level solution stage-server.sln which references all the projects in the sub-folders. Loading this solution into VS allows you to choose which targets to build, or just to build the whole solution. Note this solution only contains the components specific to server – only revdb is slightly different here (as it integrates the security component for OpenSSL) the revxml and revzip external are just the desktop ones.
*	iOS: Uses Xcode and xcode projects. The top-level project for this (that builds all components) is stage-mobile.xcodeproj which references individual xcodeproj's in each relevant folder. For example, the main engine project is engine/engine-mobile.xcodeproj.
*	Android: This is built using a script tools/build-android.osx (we're working on variants for other platforms). This script builds all the binary components and necessary Java support classes. (The binary components use the NDK native build system based on Make, so it does do dependency analysis and doesn't do a complete rebuild each time).

Prerequisites
-------------

### Mac – Xcode ###
To build the engine on Mac you need to use Xcode. Now, at the moment the 'production' system we use to build the Mac Desktop engines we distribute is Xcode 3.2.6 – this is because, at the time of writing, we still support 10.4 and PowerPC.

However, there is no need for you to necessarily use 3.2.6 – the more recent Xcode 4 can also be used but you will need to make a small change to one of the configuration files within the LiveCode repository:
* open up rules/Global.xcconfig
*	change SDKROOT to be set to 10.6

*Note:* Trevor DeVore has put together a guide for getting things to build on Mountain Lion with Xcode 4.6.2 - see [here](http://trevordevore.clarify-it.com/d/j9fxqj)

*Note:* Not using 3.2.6 and the 10.4 SDK will potentially result in engine binaries that will not run on older versions of Mac (particular PowerPC) but this doesn't matter for development purposes (only distribution).

### Linux ###
Building the engine on Linux uses a collection of makefiles and make. Our 'production' system is Ubuntu 6.06 (32-bit) as using this as a base provides a high-degree of binary compatibility between linux distributions.
In order to ensure you can build on your distribution you need to ensure you have appropriate dev packages installed. In particular:
* make 
* gcc 
* g++ 
* libX11-dev 
* libXext-dev 
* libXrender-dev 
* libXft-dev
* libXinerama-dev 
* libXv-dev 
* libXcursor-dev
* libfreetype6-dev 
* libgtk2.0-dev 
* libpopt-dev 
* libesd0-dev
* liblcms-dev 

*Note:* The above package names are for Ubuntu, other linux distributions might have slightly
different naming.

*Note:* There are still some wrinkles in building on Linux on distributions that aren't Ubuntu 6.06 but we are working with contributors to sort these out. Also, be aware of binary compatibility – compiling an engine on your Linux system in no way guarantees it will run on someone else's (different) distribution. (Good example – building the engine on Ubuntu 12.04 will cause it to crash when run on Ubuntu 6.06!).

### Windows – Visual C++ 2005 ###

At the moment you need to use Visual C++ 2005 to build the engine on Windows along with a number of other prerequisites:
*	Visual C++ Express 2005:

  http://download.microsoft.com/download/8/3/a/83aad8f9-38ba-4503-b3cd-ba28c360c27b/ENU/vcsetup.exe
*	Hotfix for Visual Studio / Visual C++ Express 2005:

  http://support.microsoft.com/kb/949009
*	Microsoft Windows Platform SDK 6.1:

  http://www.microsoft.com/en-us/download/details.aspx?id=24826
* Follow these Windows SDK integration instructions for Visual C++ Express:

  http://msdn.microsoft.com/en-us/library/ms235626(v=vs.80).aspx
*	QuickTime SDK (requires Apple Developer account to download):

  https://developer.apple.com/downloads/index.action?=quicktime
*	Microsoft Speech SDKs 4.0 + 5.1:

  http://download.microsoft.com/download/speechSDK/Install/4.0a/WIN98/EN-US/SAPI4SDK.exe
  http://download.microsoft.com/download/B/4/3/B4314928-7B71-4336-9DE7-6FA4CF00B7B3/SpeechSDK51.exe

*Note:* Using Visual C++ Express will not allow you to build revbrowser – you need the Professional edition to do that. All the other components will build though.

### iOS – Xcode ###
To build the iOS engines you need to use Xcode. For 'production' builds we use a range of versions depending on what version of iOS we are targetting (3.2.6 for 4.3, 4.2 for 5.0, 4.3.1 for 5.1 and 4.6 for 6.1). However, for development purposes any of these Xcode versions will do.

### Android ###
To build android you need the following prerequisites:
*	JDK (appropriate to your platform): http://www.oracle.com/technetwork/java/javase/downloads/index.html
*	Android SDK: http://developer.android.com/sdk/index.html

  Click “Download for Other Platforms” and select the “SDK Tools Only” package for your platform. Once downloaded and extracted, launch the SDK utility (tools/android) and install the SDK Platform package for Android 2.2 (API 8).
*	Android NDK:
  * Windows: https://dl.google.com/android/ndk/android-ndk-r6b-windows.zip
  * Mac OS X: http://dl.google.com/android/ndk/android-ndk-r6b-darwin-x86.tar.bz2 
  * Linux: http://dl.google.com/android/ndk/android-ndk-r6b-linux-x86.tar.bz2

The Android SDK and NDK need to be placed (or symlinked) to a sub-folder of your local repository. Specifically:
*	<repo>/sdks/android-ndk must point to or be the NDK folder (unarchived from above)
*	<repo>/sdks/android-sdk must point to or be the SDK folder (unarchived from above)

*Note:* At the moment we only have a script for building the Android engine on Mac (tools/build-android.osx).

Building
--------

With prerequisites installed, building the engine is quite straight-forward:
* Mac:
  * Open engine/engine.xcodeproj in Xcode and choose the 'LiveCode-Community' target
  * Select Build → Build from the menu (or press Cmd-B)
* Linux
  * From a command prompt and within your <repo> folder:
    * For debug: MODE=debug make development
    * For release: MODE=release make development
* Windows
  * Open stage.sln in Visual C++ / Visual Studio
  * Choose Project → Build Solution from the menu
* iOS
  * Open engine/engine-mobile.xcodeproj in Xcode
  * Select Build → Build from the menu (or press Cmd-B)
*  Android (Mac only!)
  * From a command prompt and within your <repo> folder type:
    * ./tools/build-android.osx

Running the engine - IDE
------------------------

When you run the IDE engine built from the git repository (from where it is built - i.e. under _build), the IDE will detect this and automatically use all the appropriate binary components that have been built alongside it. Thus, to get a fully functioning IDE with modified components all you need to do is as follows:

* Mac:
  * Build 'all' in stage.xcodeproj
  * Load the engine/engine.xcodeproj
  * Click Run/Debug.
 
* Windows:
  * Build the solution.
  * Make sure the 'engine' project is the default target.
  * Click Run.

* Linux:
  * Do 'MODE=debug make all' at top-level
  * Run '_build/linux/<arch>/debug/engine-community'

You can either choose to do a 'debug' or 'release' build - if you choose debug, then the IDE will run with all 'debug' components; if you choose release, then the IDE will run with all 'release' components.

If you wish to be able to build iOS or Android standalones with the running IDE then all you need to do is ensure those components are built too:

* iOS:
  * Build 'all' in stage-mobile.xcodeproj

* Android:
  * Do 'MODE=debug ./tools/build-android.osx'

Again, make sure you build the same type of components (debug or release) that you are running the IDE with.

*Note:* You can only build Desktop standalones for the platform you are running on when the IDE is run in this fashion.

Running the engine - Standalones
--------------------------------

Sometimes it is useful to be able to run standalones directly for debugging purposes. To do this, do the following:

For the standalone engines (only works in Debug profile):
*	Mac (Xcode 3.2.6):
  * Right click on the Standalone-Community and choose Get Info.
  * Switch to the Arguments tab.
  * Add an environment variable TEST_STACK with value the path to the stack you want to run as if it had been the mainstack that had been built into the standalone.
  * Make sure Standalone-Community is the active executable and click Build and Debug.
  * The standalone engine should now launch using the given stack.
*	Mac (Xcode 4.x):
  * Click on the target selection drop-down and select standalone.
  * Click on the target selection again and choose Edit Scheme...
  * Choose the 'Run Standalone-Community' section on the left.
  * Switch tab to Arguments
  * Make sure the Environment Variables section has a TEST_STACK variable mapping to the path to the stack you want to run as if it had been the mainstack that had been built into the standalone.
  * Click 'Run' and the engine should now launch using the specified stack.
* Linux:
  * Change directory to the top-level of the repository.
  * Create an environment variable: export TEST_STACK=<path to stack you want to use>
  * Do:
    * master branch: ./_build/linux/debug/standalone-community
    * develop branch: ./_build/linux/i386/debug/standalone-community
* Windows:
  * Right click on the standalone target and choose Properties.
  * Switch to the Debugging page of the dialog.
  * Add an environment variable TEST_STACK with value the path to the stack you want to use.
  * Make sure the standalone target is the active target and click the run button.
* iOS (Xcode 3.2.6 / Xcode 4.x):
  * Add you stack to the project so that it is included in the standalone-mobile target (otherwise it won't be placed in the app bundle where iOS can find it).
  * Search for iphone_test.livecode in the whole xcodeproj (Cmd-Shift-F).
  * Replace iphone_test.livecode with the name of the file (leaf) you added in the first step.
  * Click Build and Debug.
 
*Note:* When running the standalone engine in this fashion, none of the processes the IDE uses to build standalones are performed – you just get the given stack loaded as if it were the mainstack.

Contributing
------------

At the moment we will accept contributions to the engine and external source only. (Contributions to IDE and documentation will be organised in due course).

### Discussion ###

If you are interested in contributing to engine development then we urge you to get involved in the Engine Contributors forum on the LiveCode forums here: http://forums.runrev.com/viewforum.php?f=66

### Pull Requests ###

Our work flow is a typical git-flow model, where contributors fork the livecode repository, make their changes on a branch, and submit a Pull Request.

In general, branching should be done off of 'master' wherever possible (particularly for bug-fixes) as this allows us to be more agile in terms of the releases in which we can include changes. Obviously, in the cases where features or fixes rely on work on the current 'develop' branch then that should be the base for the branch.

When you submit a pull request, our team will review it and provide feedback to ensure the patch is suitable to be integrated into the repositoriy.

When your pull request is accepted it will be scheduled for inclusion into an appropriate release.

### Contributor's Agreements ###

If you are wanting to contribute to development of LiveCode then it is important that you sign the (Contributor's Agreement)[http://livecode.com/store/account/contributor-agreement-signup]. This agreement is required as the LiveCode project is dual-licensed both under the GPLv3 and a commercial (closed-source) license and you need to give us permission to use your submissions in this way.

*Note:* We cannot accept any pull-requests from individuals who have not signed this agreement.

### Personalising Git ###

Please ensure that your full name and email address are correctly configured in your git repository. The email address should ideally match the one you used to sign the contributor's agreement (i.e. your LiveCode Customer Login), but at the very least should match the one you use (publically) on github.

You can do this from the command-line (for all git repositories) with the following commands:

    git config --global user.name “<your name>”
    git config --global user.email <your email address>

*Note*: We cannot accept any pull-requests where the commit log does not contain correct Name / Email Address references.

### Bug Fixes ###

If you are wanting to fix a bug then please ensure there is an appropriate confirmed bug report lodged in the [LiveCode QA Centre](http://quality.runrev.com/) - this helps us track what is being fixed and provide support for doing so, should it not be straight-forward.

Pull requests for bug-fixes should be as minimal as possible, fixing only a single bug (or two or more highly-interdependent bugs).

### Features ###

If you are wanting to add a feature, or augment existing functionality we strongly encourage you to start a discussion topic about it on the [Engine Contributors](http://forums.runrev.com/viewforum.php?f=66) forum. This will help ensure that the feature you are adding is likely to be accepted, and fits in with the rest of the LiveCode language.

*Note:* In particular, we do ask that significant syntax additions be thoroughly discussed first as once added it is very hard to change and/or remove in the future!

Pull requests for features should be as minimal as possible, incorporating only the changes needed to add that feature.

### Coding Style ###

We are currently working on a more comprehensive coding style document which we will make available in due course, however here is a quick summary of our general practices in this regard:

* The majority of the engine is written in C++-as-a-better-C and so does not use the language's entire feature-set. In particular:
  * The engine does not use exceptions nor rtti.
  * The engine uses templates sparingly, typically as 'sanitized macros' for efficiency purposes, or resource acquisition classes.
  * The engine does not use the standard C++ library.
* Naming conventions:
  * Use descriptive variable and function names - don't be scared of verbosity but don't go overboard.
  * Variable names should be lower-case, using underscores to separate words.
  * Variables should be prefixed with the following to indicate their scope:
     * t_ - local variables
     * p_ - in parameters
     * r_ - out parameters
     * x_ - in-out parameters
     * m_ - object instance variables
     * s_ - object class (static) variables or file-local static variables
     * g_ - global variables
  * Function names should generally be camel-case, prefixed by 'MC' followed by the module name (note for file-local static functions, lower-case with underscores without module prefix is also acceptable).
  * Constants (both in and out of enumerations) should be camel-case prefixed by 'kMC' and the module name.
* Coding practices:
  * Declare and initialize local variables on separate lines.
  * Initialize all variables to a base value. i.e. pointers to nil, bools to true/false etc.
  * Only pass bools to if, while, do and the middle section of for - don't rely on NULL/nil/0 being false.
  * Always check the success of memory allocations, or if the calling code can't handle memory failure then mark the line that does the allocation with a /* UNCHECKED */ prefix.
  * Don't use #define's to abbreviate code.
  * Use inline functions instead of macros wherever possible.
  * 'out' function parameters should not be altered by the function until the end, and then only if the function succeeds.
  * Do not use 'goto'.
  * Do not use the ternery operator (... ? ... : ...).
  * When declaring boolean values in a struct/class use a bit-field - bool m_my_var_name : 1
* Layout and style:
  * All curly braces must be on a line on their own and indented appropriately (matching the level of the construct they are related to).
  * Use a single space after 'for', 'while', 'if', 'switch'. (e.g. if (true))
  * Do not use a space before the parameter list in function calls/definitions. (e.g. foo(a, b, c))
  * Use a single space after any comma.
  * Use a single space before and after binary operators. (e.g. x == y, x + y)
  * Put only a single statement on any line.
  * Split overly long lines appropriately (preferring the line-break after any binary operators or commas, rather than before).
  * Use a single blank line to separate different areas of code within a function.
  * Use a single blank line between function and type definitions.
  * Separate significant areas of code with a sequence of (80) slashes.

License
-------

See the [LICENSE](LICENSE) file.
