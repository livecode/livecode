# LiveCode Script Hash Functions
Copyright 2015 LiveCode Ltd.

## Introduction

Cryptographic hash functions are a key part of many applications when dealing
with security and trust issues. It is important that applications can easily
generate such hashes for binary data inputs.

As time goes by and computer power increases, existing hash functions become
weaker - typically because the ability to explicitly construct data which
produces a given hash becomes tractable. As this occurs it becomes easier to
foil security schemes by replacing trusted non-malicious data with untrusted
malicious data which hashes to the same value (thus circumventing any digital
signatures or other security mechanisms).

Due to this it is important that the range of such hash functions offered by
LiveCode Script expands as new hashes become standard.

## Taxonomy

There are a great many cryptographic hash functions which have been created
over the years. However, the ones which are most interesting (and thus should
have a special place in any library) are the ones which are intrinsically
tied to use in internet related computer security issues.

With this in mind, at the writing of this specification the following
(families of) hashes have either been, or will start to become, an important
part of internet standards:

  - md5 (first published 1991
  - sha1 (first published 1995)
  - sha2 (first published 2001)
  - sha3 (first published draft 2014 - currently being standardized)

## MD-5 Hash

The MD5 hash function is now considered to be broken and should not be used for
new applications.

LCS has had support for MD5 through the 'md5digest()' function since version 1.0
and should now only be used for code which has to deal with old protocols and
data.

### Syntax

The syntax to use the md5 hash function in LCS is:

    md5digest(<data>)
    the md5digest of <data>
    
The syntax returns a 16 byte long binary string which is the MD5 hash of the input
data.
    
### Implementation

The implementation of MD5 in LCS was written (as far as the author can tell)
by Dr Scott Raney and was based on the description in RFC 1321, and not the
reference implementation.

It can be found in engine/src/md5.c and engine/src/md5.h.

## SHA-1 Hash

The SHA-1 hash is now considered to not be secure enough for general use.

LCS has had support for SHA1 through the 'sha1digest()' function since version
4.6 and should probably be avoided in new code, and only used for backwards-compatibility.

### Syntax

The syntax to use the sha-1 hash function in LCS is:

    sha1digest(<data>)
    the sha1digest of <data>
    
The syntax returns a 20 byte long binary string which is the SHA-1 hash of the input
data.

### Implementation

The implementation of SHA-1 in LCS is Steve Reid's (sreid@sea-to-sky.net) public domain
implementation which is widely available and used through the OSS world.

It can be found in engine/src/sha1.c and engine/src/sha1.h.

## SHA-2 Hash

The SHA-2 family of hashes are the current frontier of hashes in general use for
computer security. It is still considered secure enough for general use, however
it is due to be superceeded by SHA-3 in the next few years.

The SHA-2 hash family consists of the following variants:

  - SHA-224
  - SHA-256
  - SHA-384
  - SHA-512
  - SHA-512/224
  - SHA-512/256

The SHA-256 variant has become the most generally used for computer related security
activities.

As of the time of writing, the engine has yet to have an implementation of sha-2
added.

### Syntax

The syntax for the sha-2 hash function is as follows:

    the sha2digest of <data>
    sha2digest(<data>)
    sha2digest(<data>, <form>)
  
The first two variants use SHA-256.

The third variant allows the form of SHA-2 to be specified. The <form> parameter can
be one of:

  - 224
  - 256
  - 384
  - 512
  - 512/224
  - 512/256

The various forms have the following outputs:

  - SHA-224 produces a 28 byte binary string
  - SHA-256 produces a 32 byte binary string
  - SHA-384 produces a 48 byte binary string
  - SHA-512 produces a 64 byte binary string
  - SHA-512/224 produces a 28 byte binary string
  - SHA-512/256 produces a 32 byte binary string

### Implementation

A suitably licensed implementation of the SHA-2 family is one implemented by Aaron Gifford
and can be found here http://www.aarongifford.com/computers/sha.html.

## SHA-3 Hash

The SHA-3 family of hashes are the next frontier of hashes intended to follow on from
SHA-2. At the time of writing (February 2015) NIST are putting the algorithms through the
standardization process.

Once the standardization process is complete, support for SHA-3 will be added to LCS.
