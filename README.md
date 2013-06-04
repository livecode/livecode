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

Running the engine
------------------

*Note:* At the moment running a engine you've built (whether it be the IDE or standalone engine) is a little more fiddly than we'd like – this is high on our list to sort out though!

The procedure for running the IDE or Standalone engines on each platform is all slightly different as follows...

For the IDE engines (can use either Debug or Release profiles):
* Mac (Xcode 3.2.6):
  * Right-click on the LiveCode-Community Executable and choose Get Info.
  * Switch to the Arguments tab
  * Add an environment variable REV_TOOLS_PATH with value $(SRCROOT)/../ide
  * Make sure LiveCode-Community is the active executable and click Build and Debug.
  * The engine should now launch using the ide folder within the local repo.
*	Mac (Xcode 4.x):
  * Click on the target selection drop-down and select LiveCode.
  * Click on the target selection again and choose Edit Scheme...
  * Choose the 'Run LiveCode-Community' section on the left.
  * Switch tab to Arguments
  * Make sure the Environment Variables section has a REV_TOOLS_PATH variable mapping to the full path to the ide folder within your local repo.
  * Click 'Run' and the engine should now launch using the ide within the local repo.
* Linux:
  * Change directory to the top-level of the repository.
  * Create an environment variable: export REV_TOOLS_PATH=`pwd`/ide
  * Do:
    * master branch: ./_build/linux/debug/engine-community
    * develop branch: ./_build/linux/i386/debug/engine-community
* Windows:
  * Right click on the engine target and choose Properties.
  * Switch to the Debugging page of the dialog.
  * Add an environment variable REV_TOOLS_PATH with value $(SOLUTION_DIR)ide
  * Make sure the engine target is the active target and click the run button.

*Note:* There's no easy means (at the moment) to use the externals built within the repo in the IDE when run as above. For now, the easiest thing to do is to copy the Externals folder from an existing (installed) copy of LiveCode into the ide folder inside the repo.

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

TBC

License
-------

See the [LICENSE](LICENSE) file.
