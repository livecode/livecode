# background audio on iOS

A new "experimental" feature has been added, which allows audio to continue playing while the app is on the background (i.e. when switching apps). We have added a new checkbox
"Background Audio (experimental)" in the Standalone Application Settings for iOS, which enables this feature by modifying the plist settings in the appropriate way.

The reason for this being "experimental" is that the engine does not yet support suspend/resume explicitly - this means that you do have to check your applications work correctly 
on startup / exit. In particular, if you app saves state on shutdown you might need to ensure it is saved more frequently so if your app gets terminated when in the background,
data is not lost.   
