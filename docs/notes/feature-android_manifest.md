# Android manifest merging
An android manifest merging mechanism has been added to the android
standalone builder. This enables manifests to be included in extension
jvm-android code folders which are then merged into the main manifest
at build time.

Previously, it was possible to override the _template_ manifest by 
providing a new template in a file called AndroidManifest.xml and
including it in the Copy Files list. Since the merging mechanism is 
more general, enables multiple manifests and does not require users 
to update template manifests with new template replacement strings,
this feature has been removed - instead any AndroidManifest.xml files
included in the Copy Files list will be merged into the main manifest
at build time.
