# Crash when attempting to print to file on linux.
A crash can occur when printing on linux if the engine is unable to create the file for printed output. This typically occurs if the defaultFolder has not changed to somewhere writable by the time 'print' is invoked.
