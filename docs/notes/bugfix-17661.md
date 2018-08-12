# Improve Android timestamp accuracy for GPS and sensors

Timestamps for sensors on Android were previously passed in a low-precision
format, resulting in "sticky" timestamps that did not change more than a few
times a minute. This has now been resolved and timestamps are now reported to
microsecond resolution (though the accuracy is unlikely to be at the microsecond
level).

In addition to this change, the timestamps are now reported in "monotonic" time
rather than "wall-clock" time ("wall-clock" time is the time you see reported
as the current time). This means that the timestamps are now independent of
changes to the device clock as a result of adjustments or daylight savings
changes. If you want to match the readings to the device time instead, get the
current time when receiving the location update rather than using the timestamp
in the update.

