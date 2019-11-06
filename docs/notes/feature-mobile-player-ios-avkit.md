# iOS mobile player control updated to use AVKit

The existing implementation of the mobile player control for iOS has been
replaced with one based on the AVKit framework.

While every attempt has been made to ensure full compatibility with the previous
implementation, differences between the frameworks used means that some features
may not function as before or may no longer be available. These are listed
below:

* `mobileControlDo` actions:
** "begin seeking forward" - unsupported
** "begin seeking backward" - unsupported
** "end seeking" - unsupported

* `mobileControlSet` properties:
** "useApplicationAudioSession" - unsupported
** "allowsAirPlay" - now covers all external playback

* `mobileControlGet` properties:
** "playbackState" - values "seeking forward" and "seeking backward" unsupported

* Messages:
** "playerEnterFullscreen" - enter fullscreen via user action not detected
** "playerLeaveFullscreen" - leave fullscreen via user action not detected
** "playerMovieChanged" - unsupported
