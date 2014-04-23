# Cocoa Support
With 6.7 we have replaced the majority of Carbon API usage with Cocoa. The goals of this work are three-fold:
* Allow embedding of native 'NSViews' into LiveCode windows (in particular, browser controls).
* Enable submission of LiveCode apps to the Mac AppStore.
* Enable eventual building of 64-bit versions of LiveCode for Mac.

As of DP 2 we have achieved the first goal and revBrowser has been updated as a result. The main upshot of this is that the browser is now part of the host window and as such works correctly regardless of the type of window (dialog, palette, document etc).

The instability issues caused by the AppStore sandbox when using mixed Cocoa and Carbon APIs should also be resolved in DP 2. The QuickTime and QTKit frameworks are now weakly linked and if the global property dontUseQT is set to true before any multimedia functionality is used, then these frameworks will not be loaded. This will hopefully be sufficient to allow submission of LiveCode apps to the Mac AppStore. Note that if dontUseQT is set to true, then players, sound recording and QT visual effects will not work - we will be adding AVKit based implementations of the player and sound recording in DP 3.

The final goal (64-bit support) will be gradually worked towards over the next few LiveCode versions as the engine gets 'decarbonated' (usage of Carbon APIs which do not have 64-bit equivalents removed).

As there has been quite a substantial rework on the Mac port it is expected that there will be issues to address during the release cycle. We want to ensure that the functionality of 6.7 is as close as possible to that of 6.6, so please do report any differences you notice however minor you think they might be.

With the release of dp-1 there are a number of known issues:
* No drawer support - we are currently working out how to implement this feature using Cocoa APIs
* QTVR related aspects of the player do not work - this is being worked on.

An important internal change which will affect maintainers of Mac externals that use the windowId is that this property now returns the 'global window number' (which is the unique ID the Window Server uses to identify windows). To turn this into a Cocoa NSWindow pointer use [NSApp windowWithWindowNumber: t_window_id]. Note that it is no longer possible to get a Carbon WindowRef, nor should this be attempted as trying to mix Carbon and Cocoa in this manner will cause instability inside the sandbox environment required by the Mac AppStore.

An important script visible change that has occurred due to the move to Cocoa is screen updating. Previously (when using Carbon) the OS would 'coalesce' successive requests to update the screen - the window buffer would be updated, but the window buffer would only be flushed when the OS decided to. In Cocoa, after a screen update the window buffer is *always* flushed. Outside of 'lock screen', the engine applies any screen updates after each command execution therefore in 6.7+ make sure you use lock screen around blocks of code that make many screen updates - unless you want each update to be visible. It should be noted that the behavior in 6.7 is now the same as on Windows and Linux however the OS takes longer to flush window updates to the screen on Mac than on the other platforms meaning that using lock screen is important.


