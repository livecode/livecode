#   Ability to set the dontUseQT property for a player object (Windows and OSX)

It is now possible to set the `dontUseQT` property for a player object. 

On Windows, the default value of the `dontUseQt` and `dontUseQtEffects`
(global) properties have changed from true to false. This means that by
default players created on Windows will use the DirectShow API for
multimedia playback.

On OSX, QuickTime is unable to be supported in 64 bit builds the default 
value of the `dontUseQT` (global) property changed in version 6.7 and is 
true on OS X version 10.8 and up or on all versions if the engine is 64
bit. The default value of `dontUseQTEffects` also follows this pattern. 
This means that any player object created will use the AVFoundation API 
for multimedia playback. 

With this new feature, you can set the `dontUseQT` property of a player
to false, without changing the value of the global `dontUseQt` property.
In that way you can have both QuickTime and AVFoundation players playing
at the same time. This can be particular useful for supporting some
media formats or codecs that are not supported by the default 
AVFoundation or DirectShow player (for example .midi files, Sorenson
Video 3, H.261 codecs etc) 

> *Warning*: QuickTime has not been maintained or supported by Apple for 
> quite some time and therefore it is recommended to audit your 
> applications for to remove QuickTime dependence.
