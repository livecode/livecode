# Changes to the dontUseQT property of a player object (Windows and OSX)

It is now possible to set the **dontUseQT** property for a player object. 

On Windows, the default value of the global **dontUseQt** and
**dontUseQtEffects** properties has changed from true to false. This
means that by default players created on Windows will use the
DirectShow API for multimedia playback.

On OSX, QuickTime is unable to be supported in 64 bit builds the
default value of the global **dontUseQT** and **dontUseQTEffects**
properties changed in version 6.7; it is true on OS X version 10.8 and
up, or on all versions of OS X if the engine is 64 bit.  This means
that any player object created will use the AVFoundation API for
multimedia playback.

With this new feature, you can set the **dontUseQT** property of a
player to false, without changing the value of the global
**dontUseQt** property.  If you do this, you can have both QuickTime
and AVFoundation players playing at the same time, which can be
particular useful for supporting some media formats or codecs that are
not supported by the default AVFoundation or DirectShow player (for
example .midi files, Sorenson Video 3, H.261 codecs etc)

**Warning**: QuickTime has not been maintained or supported by Apple
for quite some time.  You are encouraged to check your applications
for any dependence on QuickTime, and remove it if found.
