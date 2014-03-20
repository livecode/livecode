# Cocoa Support
With 6.7 we have replaced the majority of Carbon API usage with Cocoa. The goals of this work are three-fold:
* Allow embedding of native 'NSViews' into LiveCode windows (in particular, browser controls).
* Enable submission of LiveCode apps to the Mac AppStore.
* Enable eventual building of 64-bit versions of LiveCode for Mac.

As of DP 1 we have achieved the first goal and revBrowser has been updated as a result. The main upshot of this is that the browser is now part of the host window and as such works correctly regardless of the type of window (dialog, palette, document etc).

The instability issues caused by the AppStore sandbox when using mixed Cocoa and Carbon APIs should also be resolved in DP 1. However, the engine still statically links with QuickTime and QTKit which are now not allowed in apps submitted to the Mac AppStore. This will be addressed in DP 2 along with an AVKit implementation of the player and other multimedia features.

The final goal (64-bit support) will be gradually worked towards over the next few LiveCode versions as the engine gets 'decarbonated' (usage of Carbon APIs which do not have 64-bit equivalents removed).

As there has been quite a substantial rework on the Mac port it is expected that there will be issues to address during the release cycle. We want to ensure that the functionality of 6.7 is as close as possible to that of 6.6, so please do report any differences you notice however minor you think they might be.

With the release of dp-1 there are a number of known issues:
* No backdrop support - we are currently working out how to implement this feature using Cocoa APIs
* No drawer support - we are currently working out how to implement this feature using Cocoa APIs
* Cursor issues over window borders and during drag-drop - the cursor will sometimes stick or change to the wrong type, this is being investigated.
* Cmd-Shift-'_' does not work - this is being investigated.
* Themed scrollbars sometimes do not work correctly on Retina displays - this is being investigated.

Finally, an important internal change which will affect maintainers of Mac externals that use the windowId is that this property now returns the 'global window number' (which is the unique ID the Window Server uses to identify windows). To turn this into a Cocoa NSWindow pointer use [NSApp windowWithWindowNumber: t_window_id]. Note that it is no longer possible to get a Carbon WindowRef, nor should this be attempted as trying to mix Carbon and Cocoa in this manner will cause instability inside the sandbox environment required by the Mac AppStore.
