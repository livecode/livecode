# Player messages aren't sent correctly.
The occurrence of playStarted, playPaused and playStopped messages has been cleaned up.

The playStarted message will only be sent when the rate of the movie changes from zero to non-zero - whether via clicking the play button, setting the playRate or by using play start / play resume via script.

The playPaused message will only be sent when the rate of the movie changes from non-zero to zero - whether via clicking the pause button, setting the playRate or by using play pause / play stop.

The playStopped message will only be sent when the movie reaches the end of playback.

These are the only cases in which the messages will be sent - in particular, setting the filename will no longer send any messages and you will not get multiple messages of the same type in succession.

