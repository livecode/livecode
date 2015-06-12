#   Ability to set the dontUseQT property for a player object (windows only)

It is now possible to set the dontUseQT property for a player object. On Windows, 
the default value of the dontUseQt (global) property is false. This means that any 
player object created will use the QuickTime API for multimedia playback. With this
new feature, you can set the dontUseQT property of a player to true, without changing 
the value of the global dontUseQt property. In that way you can have both QuickTime 
and non-QuickTime players playing at the same time. 

