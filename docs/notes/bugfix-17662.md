# Improved GPS support on Android and iOS

GPS behavior is now identical on Android and iOS. On both platforms, the
location reading returned by the `mobileSensorReading` function is that
which was sent with the last system `locationChanged` event. (This 
brings iOS behavior inline with that of Android).

Additionally three new handlers have been implemented:

    mobileGetLocationHistory
    mobileSetLocationHistoryLimit
    mobileGetLocationHistoryLimit
    
Whenever a system `locationChanged` event occurs, the location reading
is pushed onto the front of a list. The list is capped at the length
set by the location history limit, dropping any old samples over this
length.

The `mobileGetLocationHistory` function returns a numerically keyed
array of all accumulated samples since the last time it was called
with lower indices being older samples. Calling the function clears
the internal history.

Each element in the array is the same format as the detailed location
array as returned from the `mobileSensorReading` function.

If an application wants historical access to all samples, then it
should set the location history limit to the maximum number of samples
it ever wants to record, or 0 to record the entire history (between
calls to `mobileGetLocationHistory`).

The best way to use the history is to fetch the list in `locationChanged`
and process each sample in turn, rather than the sample provided
with the `locationChanged` event (which will always be the last sample
in the history). e.g.

    on locationChanged
       local tHistory
       put mobileGetLocationHistory() into tHistory
       repeat for each element tSample in tHistory
          processLocationChanged tSample
       end repeat
    end locationChanged
    
The default history limit is 1 meaning that only one sample is
ever kept at a time.
