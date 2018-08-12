# Include .jar files from extensions code folders
The standalone builder will now look for jar files in 
`<path to extension>/code/jvm-android` and update the `jarFiles` 
standalone setting accordingly when building for android. It will 
also look for them in `<path to extension>/code/jvm` for code 
inclusions on all platforms that support java.

So in order to include compile java classes in an extension and
access them from LCB, simply put the .jar files into the appropriate
folder (`code/jvm-android` if the jar depends on the android API,
and `code/jvm` otherwise). 
