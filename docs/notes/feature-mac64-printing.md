# Update the OSX printer code for 64-bit compatibility

Due to changes in Apple's APIs for printing, there are some (very small)
differences in printing behaviour. In 64-bit engines, no dialogue will
be displayed while spooling to the printer as Apple no longer provides
it.

Additionally, the API previously used to implement the 'printerSettings'
property was deprecated by Apple and has been updated to use the new
API and compatibility between the binary strings returned by the two
APIs cannot be guaranteed.

