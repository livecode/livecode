# command mobileSetUseAndroidAudioWorkaround

The [bug 15134](http://quality.runrev.com/show_bug.cgi?id=15134 "Bug 15134") happens to be caused by the same reason causing the [bug 10437](http://quality.runrev.com/show_bug.cgi?id=10437 "Bug 10437"), this time affecting Nexus 7 device.

Since the issue is a device-specific problem, and is linked to the Android Completion Listener feature firing too early to let the whole sound play, we added a new command
`mobileSetUseAndroidAudioWorkaround`
to allow LiveCodera to cope with the issue depending on the device on which the application is running.

Using `mobileSetUseAndroidAudioWorkaround true` will make use of a patched MediaPlayer, which delays the call to the Completion Listener API, and thus avoid the sound to be cropped.

We also added the function `mobileGetUseAndroidAudioWorkaround` in order to get the current configuration of the engine.

More information is available from the dictionary.
