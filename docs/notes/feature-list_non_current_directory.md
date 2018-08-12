# List folders other than the default folder

When called as a function, the **files** and **folders** functions
now take an optional argument specifying which directory to list.
This makes writing filesystem code a lot easier, since code that
looked like:

    local tOldFolder, tFilesList
    put the defaultFolder into tOldFolder
    set the defaultFolder to "/path/to/target/directory"
    put the files into tFilesList
    set the defaultFolder to tOldFolder
    return tFilesList

can be replaced with:

    return files("/path/to/target/directory")
