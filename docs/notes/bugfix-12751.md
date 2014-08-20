# QT-related features don't work.
QT effects and sound recording will now work as long as 'dontUseQT' is set to false. In this case, the player will default to using QTKit.
If you are submitting an app to the Mac AppStore, or wish to use AVFoundation player on 10.8 and above, ensure that dontUseQT is set to true in your startup handler, or before any code or stack which uses QT is run.
