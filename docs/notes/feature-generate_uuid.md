# UUID Generation
There is a new function **uuid()** which can be used to generate UUIDs on all platforms:

	**uuid([ *type*, [ *namespace_id*, *name* ]])**

If no parameters are specified, or *type* is "random" then it returns a version 4 (random) UUID. A cryptographic quality pseudo-random number generator is used to generate the randomness.

If *type* is "md5" or "sha1" then it returns a version 3 (md5) or version 5 (sha1) UUID. Here *namespace_id* should be the UUID of the namespace in which *name* sits, and *name* can be any string.
