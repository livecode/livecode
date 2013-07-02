# Random Number Generation
The **randomBytes()** function has been updated to use the platform-provided cryptographic-quality pseudo-number-generator in the case that the SSL libraries are not available. This change means that this function is now always available on all platforms.
