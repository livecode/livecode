#  New iOS status bar style

**What has changed?**

The way the status bar is displayed changed in iOS 7. Previously, if the status bar was visible,
the app view was shifted down by height of the status bar (20 pixels). From iOS 7 and above,
the app view is given these 20 pixels of extra height, and the status bar is displayed on top of
the app view (i.e there is an overlapping). This is a feature added by Apple, to give developers
control over what appears behind the content of the status bar.

Some users reported this change of behaviour as a bug, since it changed the way their existing
stacks were displayed in iOS 7 and above. A quick fix to this would be updating the engine to move
the app view down 20 pixels, if the status bar is opaque. However, this fix would not guarantee
backwards compatibility, since some users may have already adjusted for iOS 7 status bar behaviour,
and have modified their code to work with those changes.

So we decided to add a new **solid** status bar style, which is opaque and automatically shifts down
the view content by 20 pixels. So the difference between “opaque” and “solid” status bar styles is
the following:

        | pre-iOS 7       | iOS 7+  
---------|---------|----------------------------------------------
  opaque    | move the stack below the status bar | the status bar will be over the top 
  solid    | move the stack below the status bar  |   move the stack below the status bar 
