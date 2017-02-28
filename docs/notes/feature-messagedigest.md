# New messageDigest() function with SHA-2 and SHA-3 support

A new `messageDigest()` function has been added.  It allows access to
a variety of cryptographic message digest functions, including SHA-2
and SHA-3.  For example, to compute the 256-bit SHA-3 digest of the
message "LiveCode", you might use:

    get messageDigest(textEncode("LiveCode", "UTF-8"), "sha3-256")
