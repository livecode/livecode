# [Global Jam] Can't deploy to iOS 9 Simulator when using referenced images

To improve deployment speed on iOS simulator, the files referenced in the Standalone Settings 'Copy Files' pane are not actually copied, but symlinked.
Unfortunately, it seems that iOS Simulator 9.0 does not handle this the way the previous simulators do, and when deploying to iOS Simulator 9.0, the files have to be copied.
Therefore, expect a longer deployment building to iOS simulator 9.0 if you have big files attached to your application.
