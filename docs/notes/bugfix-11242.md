# iOS Native Scroller doesn't work correctly on iOS 7
Due to a bug in the UIScrollView in iOS 7 and above, the 'delayTouches' property of the native scroller object was not working correctly. A work-around for the OS bug has been put in place that should emulate its pre-iOS7 functionality.
