# Setup
## Installation

Each version of LiveCode installs can be installed to its own,
separate folder.  This allow multiple versions of LiveCode to be
installed side-by-side.  On Windows (and Linux), each version of
LiveCode has its own Start Menu (or application menu) entry. On Mac OS
X, each version has its own app bundle.

On Mac OS X, install LiveCode by mounting the `.dmg` file and dragging
the app bundle to the `Applications` folder (or any other suitable
location).

For Windows and Linux, the default installation locations when
installing for "All Users" are:

| Platform | Path |
| -------- | ---- |
| Windows  | `<x86 program files folder>/RunRev/LiveCode <version>` |
| Linux    | `/opt/livecode/livecode-<version>` |

The installations when installing for "This User" are:

| Platform | Path |
| -------- | ---- |
| Windows  | `<user roaming app data folder>/RunRev/Components/LiveCode <version>` |
| Linux    | `~/.runrev/components/livecode-<version>` |

**Note:** If installing for "All Users" on Linux, either the **gksu** tool must be available, or you must manually run the LiveCode installer executable as root (e.g. using **sudo** or **su**).

## Uninstallation

On Windows, the installer hooks into the standard Windows uninstall mechanism. This is accessible from the "Add or Remove Programs" applet in the windows Control Panel.

On Mac OS X, drag the app bundle to the Trash.

On Linux, LiveCode can be removed using the `setup.x86` or `setup.x86_64` program located in LiveCode's installation directory.

## Reporting installer issues

If you find that the installer fails to work for you then please report it using the [LiveCode Quality Control Centre](http://quality.livecode.com) or by emailing support@livecode.com.

Please include the following information in your report:

* Your platform and operating system version
* The location of your home or user folder
* The type of user account you are using (guest, restricted, admin etc.)
* The installer log file.

The installer log file can be located as follows:

| Platform        | Path  |
| --------------- | ----- |
| Windows 2000/XP | `<documents and settings folder>/<user>/Local Settings/` |
| Windows Vista/7 | `<users folder>/<user>/AppData/Local/RunRev/Logs` |
| Linux           | `<home>/.runrev/logs` |

## Activating LiveCode Indy or Business edition

The licensing system ties your product licenses to a customer account system, meaning that you no longer have to worry about finding a license key after installing a new copy of LiveCode. Instead, you simply have to enter your email address and password that has been registered with our customer account system and your license key will be retrieved automatically.

Alternatively it is possible to activate the product via the use of a specially encrypted license file. These will be available for download from the customer center after logging into your account. This method will allow the product to be installed on machines that do not have access to the internet.

## Command-line installation

It is possible to invoke the installer from the command-line on Linux and Windows. When doing command-line installation, no GUI will be displayed.  The installation process is controlled by arguments passed to the installer.

Run the installer using a command in the form:

	<installer> install -ui [OPTION ...]

where `<installer>` should be replaced with the path of the installer executable or app (inside the DMG) that has been downloaded.  The result of the installation operation will be written to the console.

The installer understands any of the following `OPTION`s:

| Option  | Description  |
| ------- | ------------ |
|`-allusers`          | Install the IDE for "All Users". If not specified, LiveCode will be installed for the current user only. |
|`-desktopshortcut`   | Place a shortcut on the Desktop (Windows-only) |
|`-startmenu`         | Place shortcuts in the Start Menu (Windows-only) |
|`-location LOCATION` | The folder to install into. If not specified, the `LOCATION` defaults to those described in the "Installation" section above. |
|`-log LOGFILE`       | The file to which to log installation actions. If not specified, no log is generated. |

**Note:** the command-line installer does not do any authentication. When installing for "All Users", you will need to run the installer command as an administrator.

As the installer is actually a GUI application, it needs to be run slightly differently from other command-line programs.

On Windows, the command is:

	start /wait <installer> install -ui [OPTION ...]

## Command-line uninstallation

It is possible to uninstall LiveCode from the command-line on Windows and Linux.  When doing command-line uninstallation, no GUI will be displayed.

Run the uninstaller using a command of the form:

	<uninstaller> uninstall -ui

Where <exe> is *.setup.exe* on Windows, and *.setup.x86* on Linux. This executable, for both of the platforms, is located in the folder where LiveCode is installed.

The result of the uninstallation operation will be written to the console.

**Note:** the command-line uninstaller does not do any authentication.  When removing a version of LiveCode installed for "All Users", you will need to run the uninstaller command as an administrator.

## Command-line activation for LiveCode Indy or Business edition

It is possible to activate an installation of LiveCode for all users by using the command-line.  When performing command-line activation, no GUI is displayed.  Activation is controlled by passing command-line arguments to LiveCode.

Activate LiveCode using a command of the form:

	<livecode> activate -file LICENSEFILE -passphrase SECRET

where `<livecode>` should be replaced with the path to the LiveCode executable or app that has been previously installed.

This loads license information from the manual activation file `LICENSEFILE`, decrypts it using the given `SECRET` passphrase, and installs a license file for all users of the computer.  Manual activation files can be downloaded from the [My Products](https://livecode.com/account/products/livecode) page in the LiveCode account management site.

It is also possible to deactivate LiveCode with:

	<livecode> deactivate

Since LiveCode is actually a GUI application, it needs to be run slightly differently from other command-line programs.

On Windows, the command is:

	start /wait <livecode> activate -file LICENSE -passphrase SECRET
	start /wait <livecode> deactivate

On Mac OS X, you need to do:

	<livecode>/Contents/MacOS/LiveCode activate -file LICENSE -passphrase SECRET
	<livecode>/Contents/MacOS/LiveCode deactivate

