# Make filenames with accented characters appear correctly in the detailed files on Mac

The detailed files uses URL encoding to be able to present the filename in a comma-
separated list. However, this causes a problem because the current definition or url
encode and decode in LiveCode is that it %-encodes the native encoding of the string.

As Mac HFS(+) volumes store filenames in decomposed unicode form, this means that
any filenames containing accented characters will not appear correctly in the
detailed files, even if the combined character is in the native character set.

To improve this situation, the detailed files will now normalize all filenames to
composed form before url encoding.
