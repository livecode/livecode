# New `log` command and `logMessage` property

A new command (`log`) and global property (`logMessage`) have been added to allow
an easy and low-cost method to disable or redirect script logs.

The `log` command invokes the handler named by the `logMessage` as though the
`logMessage` were directly written in the script. For backwards compatability
the default value of the `logMessage` is `log` so any scripts that currently
have a `log` handler will continue to work. To allow this `log` has been special
cased as both a command name and a permitted handler name.

If the `logMessage` is set to empty then the `log` command will not invoke any
handler or evaluate any of the parameters in the argument list.

In this example the `log` command will not be called with `pInfo` as
`loading resources` when the `uBuildMode` of the stack is `release`:

    on preOpenStack
       -- uBuildMode property set before building standalone
       if the uBuildMode of this stack is "release" then
          set the logMessage to empty
       end if
       
       loadResources
    end preOpenStack

    command loadResources
       log "loading resources"
    end loadResources

    on log pInfo
       -- unhandled put will go to system logs
       put pInfo
    end log