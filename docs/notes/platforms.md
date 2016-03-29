# Platform support
The engine supports a variety of operating systems and versions. This section describes the platforms that we ensure the engine runs on without issue (although in some cases with reduced functionality).
## Windows
The engine supports the following Windows OSes:
*Windows XP SP2 and above
*Windows Server 2003
*Windows Vista SP1 and above (both 32-bit and 64-bit)
*Windows 7 (both 32-bit and 64-bit)
*Windows Server 2008
*Windows 8.x (Desktop)
* **Note:** On 64-bit platforms the engine still runs as a 32-bit application through the WoW layer.*
## Linux
The linux engine requires the following:
*32-bit installation, or a 64-bit linux distribution that has a 32-bit compatibility layer
*2.4.x or later kernel
*X11R5 capable Xserver running locally on a 24-bit display
*glibc 2.3.2 or later
*gtk/gdk/glib (optional – required for native theme support)
*pango/xft (optional – required for pdf printing, anti-aliased text and unicode font support)
*lcms (optional – required for color profile support in JPEGs and PNGs)
*gksu (optional – required for elevate process support)
* **Note:** The optional requirements (except for gksu and lcms) are also required by Firefox and Chrome, so if your linux distribution runs one of those, it will run the engine.*
* **Note:** If the optional requirements are not present then the engine will still run but the specified features will be disabled.*
* **Note:** LiveCode and standalones it builds may work on remote Xservers and in other bit-depths, however this mode of operation is not currently supported.*
## Mac
The Mac engine supports:
*10.6.x (Snow Leopard) on Intel
*10.7.x (Lion) on Intel
*10.8.x (Mountain Lion) on Intel
*10.9.x (Mavericks) on Intel
*10.10.x (Yosemite) on Intel
*10.11.x (El Capitan) on Intel
* **Note:** The engine runs as a 32-bit application regardless of the capabilities of the underlying processor.*
## iOS
iOS deployment is possible when running LiveCode IDE on a Mac, and provided Xcode is installed and has been set in LiveCode *Preferences* (in the *Mobile Support* pane).
Currently, the supported versions of Xcode are:
*Xcode 4.6 on MacOS X 10.7
*Xcode 5.1 on MacOS X 10.8
*Xcode 6.2 on MacOS X 10.9
*Xcode 6.2 and 7.2 on Mac OS X 10.10
*Xcode 7.3 on MacOS X 10.11

It is also possible to set other versions of Xcode, to allow testing on a wider range of iOS simulators. For instance, on Yosemite, you can add *Xcode 5.1* in the *Mobile Support* preferences, to let you test your stack on the *iOS Simulator 7.1*.
We currently support the following iOS Simulators:
*5.1
*6.1
*7.1
*8.2
*9.2
*9.3