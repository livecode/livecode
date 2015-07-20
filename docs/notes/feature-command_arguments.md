# the *commandName* and the *commandArguments*

The bugs [12106](http://quality.runrev.com/show_bug.cgi?id=12106) and [12018](http://quality.runrev.com/show_bug.cgi?id=12108) have been longstanding issues: *$0* is not the command name, but the first command argument (which does not follow the way Bash works.

To solve this issue and avoid breaking any script that uses the current way command arguments are retrieved wth LiveCode (*$0* being the first commandline arguments instead of the executable name), we introduced two functions to allow the users to access the commandline name and parameters.

* the `commandName` returns the command that has been used to start the executable
* the `commandArguments` returns the list of the command arguments, on argument per line.

These functions are only implemented for desktop standalone applications and server scripts. They will return *empty* on mobile platforms and in the IDE.
