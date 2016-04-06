# revZipOpenArchive can fail on 64-bit Linux
The revZipOpenArchive command can fail to open a valid zip archive on 64-bit versions of Linux. This was due to a 64-bit cleanliness problem in the libzip library which has now been fixed.
