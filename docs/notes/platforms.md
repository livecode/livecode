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
*Supported architectures:
	*32-bit or 64-bit Intel/AMD or compatible processor
	*32-bit ARMv6 with hardware floating-point (e.g. RaspberryPi)
*Common requirements for GUI functionality:
	*GTK/GDK/Glib 2.24 or later
	*Pango with Xft support
	* *(optional)* esd - required for audio output
	* *(optional)* mplayer - required for media player functionality
	* *(optional)* lcms - required for color profile support in images
	* *(optional)* gksu - required for privilege elevation support
*Requirements for 32-bit Intel/AMD:
	*glibc 2.3.6 or later
*Requirements for 64-bit Intel/AMD:
	*glibc 2.15 or later
*Requirements for ARMv6:
	*glibc 2.7 or later
* **Note:** The GUI requirements are also required by Firefox and Chrome, so if your Linux distritution runes one of those, it will run the engine.*
* **Note:** If the optional requirements are not present then the engine will still run but the specified features will be disabled.*
* **Note:** It may be possible to compile and run LiveCode Community on other architectures but this is not officially supported.*
## Mac
The Mac engine supports:
*10.6.x (Snow Leopard) on Intel
*10.7.x (Lion) on Intel
*10.8.x (Mountain Lion) on Intel
*10.9.x (Mavericks) on Intel
* **Note:** The engine runs as a 32-bit application regardless of the capabilities of the underlying processor.*
