# the *commandName* and the *commandArguments*

The bugs [12106](http://quality.runrev.com/show_bug.cgi?id=12106) and [12018](http://quality.runrev.com/show_bug.cgi?id=12108) have been longstanding issues: *$0* is not the command name, but the first command argument (which does not follow the way Bash works).

To solve this issue and avoid breaking any script that uses the current way command arguments are retrieved wth LiveCode (*$0* being the first commandline argument instead of the command name), we introduced two functions to allow the users to access the commandline name and arguments.

* the `commandName` returns the command that has been used to start the executable
* the `commandArguments` returns a 1-based, numeric array of the commandline arguments if no index is given. Returns the arguments at this index otherwise (or empty if the index is < 1 or > number of parameters)

These functions are only implemented for desktop standalone applications and server scripts. They will return *empty* on mobile platforms and in the IDE.
