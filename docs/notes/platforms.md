# Platform support
The engine supports a variety of operating systems and versions. This section describes the platforms that we ensure the engine runs on without issue (although in some cases with reduced functionality).

## Windows

LiveCode supports the following versions of Windows:

* Windows XP SP2 and above
* Windows Server 2003
* Windows Vista SP1 and above (both 32-bit and 64-bit)
* Windows 7 (both 32-bit and 64-bit)
* Windows Server 2008
* Windows 8.x (Desktop)
* Windows 10

**Note:** On 64-bit Windows installations, LiveCode runs as a 32-bit application through the WoW layer.

## Linux

LiveCode supports Linux installations which meet the following requirements:

* Supported CPU architectures:
  * 32-bit or 64-bit Intel/AMD or compatible processor
  * 32-bit ARMv6 with hardware floating-point (e.g. RaspberryPi)

* Common requirements for GUI functionality:
  * GTK/GDK/Glib 2.24 or later
  * Pango with Xft support
  * esd (optional, needed for audio output)
  * mplayer (optional, needed for media player functionality)
  * lcms (optional, required for color profile support in images)
  * gksu (optional, required for privilege elevation support)

* Requirements for 32-bit Intel/AMD:
  * glibc 2.11 or later
* Requirements for 64-bit Intel/AMD:
  * glibc 2.13 or later
* Requirements for ARMv6:
  * glibc 2.7 or later

**Note:** If the optional requirements are not present then LiveCode will still run but the specified features will be disabled.

**Note:** The requirements for GUI functionality are also required by Firefox and Chrome, so if your Linux distribution runs one of those, it will run LiveCode.

**Note:** It may be possible to compile and run LiveCode Community for Linux on other architectures but this is not officially supported.

## Mac
The Mac engine supports:

* 10.6.x (Snow Leopard) on Intel
* 10.7.x (Lion) on Intel
* 10.8.x (Mountain Lion) on Intel
* 10.9.x (Mavericks) on Intel
* 10.10.x (Yosemite) on Intel
* 10.11.x (El Capitan) on Intel

## iOS
iOS deployment is possible when running LiveCode IDE on a Mac, and provided Xcode is installed and has been set in LiveCode *Preferences* (in the *Mobile Support* pane).

Currently, the supported versions of Xcode are:
* Xcode 4.6 on MacOS X 10.7
* Xcode 5.1 on MacOS X 10.8
* Xcode 6.2 on MacOS X 10.9
* Xcode 6.2, 6.4, 7.1 and 7.2 on Mac OS X 10.10
* Xcode 7.1 and 7.2 on MacOS X 10.11

It is also possible to set other versions of Xcode, to allow testing on a wider range of iOS simulators. For instance, on Yosemite, you can add *Xcode 5.1* in the *Mobile Support* preferences, to let you test your stack on the *iOS Simulator 7.1*.

We currently support the following iOS Simulators:

* 6.1
* 7.1
* 8.2
* 8.4
* 9.2

## Android

LiveCode allows you to save your stack as an Android application, and
also to deploy it on an Android device or simulator from the IDE.

Android deployment is possible from Windows, Linux and Mac OSX.

To enable deployment to Android devices, you need to download the
[Android SDK](https://developer.android.com/sdk/index.html#Other), and
then use the 'Android SDK Manager' to install:

* the latest "Android SDK Tools"
* the latest "Android SDK Platform Tools"

You also need to install the Java Development Kit (JDK).  On Linux,
this usually packaged as "openjdk".  LiveCode requires JDK version 1.6
or later.

Once you have set the path of your Android SDK in the "Mobile Support"
section of the LiveCode IDE's preferences, you can deploy your stack
to Android devices.

Some users have reported successful Android Watch deployment, but it
is not yet officially supported.

## HTML5

LiveCode applications can be deployed to run in a web browser, by running the LiveCode engine in JavaScript and using modern HTML5 JavaScript APIs.

HTML5 deployment does not require any additional development tools to be installed.

LiveCode HTML5 standalone applications are currently supported for running in recent versions of [Mozilla Firefox](https://www.mozilla.org/firefox/new/), [Google Chrome](https://www.google.com/chrome/) or [Safari](https://support.apple.com/HT204416).  For more information, please see the "HTML5 Deployment" guide in the LiveCode IDE.
