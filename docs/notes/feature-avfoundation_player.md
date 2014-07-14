# Multimedia on MacOS with AVFoundation 

**What has changed?**

The player object until now used QuIckTime/QTKit APIs for audio/video playback. Since both QuickTime and QTKit are deprecated by Apple, we moved to use the newer Apple multimedia APIs, i.e. AVFoundation. Moreover, since AVFoundation does not support a controller for multimedia playback until OSX 10.9, and some features of the old QTKIt controller were broken, we implemented our controller. We added two new properties to the player object, that are related to the appearance of the custom controller:

- The **hilitecolor** of a player is the color of the played area, the colour of the volume area, as well as the background color of a controller button when it is pressed.

- The **forecolor** of a player is the color of the selected area. The selected area is the area between the selection handles.

Note that AVFoundation is available from OSX 10.7 and above. However, the new AVFoundation player is supported in OSX 10.8 and above. This means that in machines that run OSX 10.7, LC 6.7-dp5 will still use the old QTKit player. This is because some features of the player (such as the alwaysBuffer property) could be supported only by using AVFoundation classes that are available in OSX 10.8 and above. 