# Setup
## Installation
Each distinct version has its own complete folder – multiple versions will no longer install side-by-side: on Windows (and Linux), each distinct version will gain its own start menu (application menu) entry; on Mac, each distinct version will have its own app bundle.
The default location for the install on the different platforms when installing for 'all users' are:
*Windows: <x86 program files folder>/RunRev/LiveCode <version>
*Linux: /opt/runrev/livecode-<version>
*Mac: /Applications/LiveCode <version>.app
The default location for the install on the different platforms when installing for 'this user' are:
*Windows: <user roaming app data folder>/RunRev/Components/LiveCode <version>
*Linux: ~/.runrev/components/livecode-<version>
*Mac: ~/Applications/LiveCode <version>.app
* **Note:** If your linux distribution does not have the necessary support for authentication (gksu) then the installer will run without admin privileges so you will have to manually run it from an admin account to install into a privileged location.*
## Uninstallation
On Windows, the installer hooks into the standard Windows uninstall mechanism. This is accessible from the appropriate pane in the control panel.
On Mac, simply drag the app bundle to the Trash.
On Linux, the situation is currently less than ideal:
*open a terminal
**cd* to the folder containing your LiveCode install. e.g.
	`cd /opt/runrev/livecode-<version>`
*execute the *.setup.x86* file. i.e.
	`./.setup.x86`
*follow the on-screen instructions.
# Reporting installer issues
If you find that the installer fails to work for you then please file a bug report in the RQCC or email support@livecode.com so we can look into the problem.
In the case of failed install it is vitally important that you include the following information:
*Your platform and operating system version
*The location of your home/user folder
*The type of user account you are using (guest, restricted, admin etc.)
*The installer log file located as follows:
* **Windows 2000/XP:** <documents and settings folder>/<user>/Local Settings/
* **Windows Vista/7:** <users folder>/<user>/AppData/Local/RunRev/Logs
* **Linux:** <home>/.runrev/logs
* **Mac:** <home>/Library/Application Support/Logs/RunRev
# Activation
The licensing system ties your product licenses to a customer account system, meaning that you no longer have to worry about finding a license key after installing a new copy of LiveCode. Instead, you simply have to enter your email address and password that has been registered with our customer account system and your license key will be retrieved automatically. 
Alternatively it is possible to activate the product via the use of a specially encrypted license file. These will be available for download from the customer center after logging into your account. This method will allow the product to be installed on machines that do not have access to the internet.
# Multi-user and network install support (4.5.3)
In order to better support institutions needing to both deploy the IDE to many machines and to license them for all users on a given machine, a number of facilities have been added which are accessible by using the command-line.
* **Note:** These features are intended for use by IT administrators for the purposes of deploying LiveCode in multi-user situations. They are not supported for general use.*
# Command-line installation
It is possible to invoke the installer from the command-line on both Mac and Windows. When invoked in this fashion, no GUI will be displayed, configuration being supplied by arguments passed to the installer.
On both platforms, the command is of the following form:
	<exe> install noui *options*
Here *options* is optional and consists of one or more of the following:

|-allusers 		|	Install the IDE for all users. If not specified, the install will be done for the current user only.|
|-desktopshortcut	|     	Place a shortcut on the Desktop (Windows-only) |
|-startmenu           	|	Place shortcuts in the Start Menu (Windows-only)|
|-location *location*	| 	The location to install into. If not specified, the location defaults to those described in the *Layout* section above.|
|-log *logfile*       	|	A file to place a log of all actions in. If not specified, no log is generated.|
Note that the command-line variant of the installer does not do any authentication. Thus, if you wish to install to an admin-only location you will need to be running as administrator before executing the command.
As the installer is actually a GUI application, it needs to be run slightly differently from other command-line programs.
In what follows <installerexe> should be replaced with the path of the installer executable or app (inside the DMG) that has been downloaded.
On Windows, you need to do:
	start /wait <installerexe> install noui *options*
On Mac, you need to do:
	“<installerexe>/Contents/MacOS/installer” install noui *options*
On both platforms, the result of the installation will be written to the console.
# Command-line activation
In a similar vein to installation, it is possible to activate an installation of LiveCode for all-users of that machine by using the command-line. When invoked in this fashion, no GUI will be displayed, activation being controlled by any arguments passed.
On both platforms, the command is of the form:
	<exe> activate -file *license* -passphrase *phrase*
This command will load the manual activation file from *license*, decrypt it using the given *passphrase* and then install a license file for all users of the computer. Manual activation files can be downloaded from the 'My Products' section of the LiveCode customer accounts area.
This action can be undone using the following command:
	<exe> deactivate
Again, as the LiveCode executable is actually a GUI application it needs to be run slightly differently from other command-line programs.
In what follows <livecodeexe> should be replaced with the path to the installed LiveCode executable or app that has been previously installed.
On Windows, you need to do:
	start /wait <livecodeexe> activate -file *license* -passphrase *phrase*
	start /wait <livecodeexe> deactivate
On Mac, you need to do:
	“<livecodeexe>/Contents/MacOS/LiveCode” activate -file *license* -passphrase *phrase*
	“<livecodeexe>/Contents/MacOS/LiveCode” deactivate
On both platforms, the result of the activation will be written to the console.