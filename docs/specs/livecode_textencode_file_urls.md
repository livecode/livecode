# LiveCode Script - `file+<encoding>` URL support
Copyright 2015 LiveCode Ltd.


## Introduction

LiveCode can access files through the URL syntax in two different ways: the `binfile:` scheme for binary data access and the `file:` scheme for text data.

Unfortunately, backwards compatibility constraints mean that the `file:` scheme treats files as being encoded using the "native" encoding for the platform it is being run on. If the file has some other encoding (for example, UTF-8) the procedure for loading that file as text is slightly cumbersome:

	put url("binfile:file.txt") into tEncodedText
	put textDecode(tEncodedText, "UTF-8") into tText

Automatic detection as been suggested as a workaround; this is not only unreliable but it is also redundant if the encoding is known ahead of time. A mechanism to load a text file using the URL syntax using a specified text encoding is required.

## Proposed Fix

The proposed mechanism is to allow text files to be loaded using a `file+<encoding>` URL scheme, for example `file+utf8:`.  

The encoding is all characters between the `+` symbol and the next `:` symbol. If the name of the encoding is empty, the `+` sign may be omitted and the encoding is unspecified. If the encoding is not specified, the `file:` URL scheme will maintain its existing, backwards-compatible, behaviour. 

The valid values for the `<encoding>` part of the scheme are a restricted form of those allowed as the encoding parameter for the `textEncode` and `textDecode` functions: the characters must be valid in the scheme component of a URL. Because the `+` character is being used as a prefix, all characters other than `[a-zA-Z0-9]`, `.` and `-` must be stripped from the encoding name. The names of all encodings supported by the engine already conform to these rules.

The same canonicalisation rules as used for the encoding parameter to the `textEncode`/`textDecode` functions will be used for interpreting the name of the encoding; this means that, for example, `file+utf8:` and `file+UTF-8:` have identical meaning.

