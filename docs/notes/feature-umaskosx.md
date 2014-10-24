# Enable "umask" property on OS X

On POSIX systems, it is sometimes useful to set the umask when creating files or
    directories.  For example, this can be useful when creating temporary
    directories.

Previously, the "umask" property in LiveCode was only implemented on iOS, Linux
    and Android platforms.  It is now also available on Mac OS X.