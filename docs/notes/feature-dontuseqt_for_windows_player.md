#   Ability to set the dontUseQT property for a player object (Windows and OSX)

It is now possible to set the dontUseQT property for a player object. On Windows, 
the default value of the dontUseQt (global) property is false. This means that any 
player object created will use the QuickTime API for multimedia playback. With this
new feature, you can set the dontUseQT property of a player to true, without changing 
the value of the global dontUseQt property. In that way you can have both QuickTime 
and non-QuickTime players playing at the same time. 

On OSX, the default value of the dontUseQT (global) property is true if the OSX version is 
greater or equal to 10.8. This means that any player object created will use the AVFoundation
API for multimedia playback. With this new feature, you can set the dontUseQT property of a
player to false, without changing the value of the global dontUseQt property. In that way you 
can have both QuickTime and AVFoundation players playing at the same time. This can be particular 
useful for supporting some media formats or codecs that are not supported by the default AVFoundation player
(for example .midi files, Sorenson Video 3, H.261 codecs etc) 


