# Android Hardware Acceleration

In order to have the Native Android Browser object being able to play videos, the Hardware Acceleration of the activity must be enabled.

However, activating the hardware acceleration rendering decreases the stack rendering speed considerably due to our use of a frequently changing bitmap as the stack view.

Thus, we added an option in the Standalone Settings to let users choose between a fast-rendering application (without the Hardware Acceleration) or an application whose Native Browsers can play videos.
