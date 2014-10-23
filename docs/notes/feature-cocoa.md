# Cocoa Support
With 6.7 we have replaced the majority of Carbon API usage with Cocoa. The goals of this work are three-fold:
* Allow embedding of native 'NSViews' into LiveCode windows (in particular, browser controls).
* Enable submission of LiveCode apps to the Mac AppStore.
* Enable eventual building of 64-bit versions of LiveCode for Mac.

We have achieved the first two of these goals in 6.7.

The instability issues caused by the AppStore sandbox when using mixed Cocoa and Carbon APIs has been resolved - LiveCode apps built with 6.7 can be successfully sandboxed and thus submitted to the AppStore.

The dontUseQT property is now true by default on Mac. This means that, by default, the AVKit implementation of the player will be used on 10.8 and above. Note that, as it stands, when dontUseQT is true neither QT visual effects nor sound recording will work.

The final goal (64-bit support) will be gradually worked towards over the next few LiveCode versions as the engine gets 'decarbonated' (usage of Carbon APIs which do not have 64-bit equivalents removed).

An important internal change which will affect maintainers of Mac externals that use the windowId is that this property now returns the 'global window number' (which is the unique ID the Window Server uses to identify windows). To turn this into a Cocoa NSWindow pointer use [NSApp windowWithWindowNumber: t_window_id]. Note that it is no longer possible to get a Carbon WindowRef, nor should this be attempted as trying to mix Carbon and Cocoa in this manner will cause instability inside the sandbox environment required by the Mac AppStore.

An important script visible change that has occurred due to the move to Cocoa is screen updating. Previously (when using Carbon) the OS would 'coalesce' successive requests to update the screen - the window buffer would be updated, but the window buffer would only be flushed when the OS decided to. In Cocoa, after a screen update the window buffer is *always* flushed. Outside of 'lock screen', the engine applies any screen updates after each command execution therefore in 6.7+ make sure you use lock screen around blocks of code that make many screen updates - unless you want each update to be visible. It should be noted that the behavior in 6.7 is now the same as on Windows and Linux however the OS takes longer to flush window updates to the screen on Mac than on the other platforms meaning that using lock screen is important.

Note: QTVR movies are no longer supported as they are not supported by QTKit nor AVKit.

Note: Drawers no longer work on Mac, they will appear as normal stacks.
