# Unicode Support

# Unicode and LiveCode

Traditionally, computer systems have stored text as 8-bit bytes, with each byte representing a single character (for example, the letter 'A' might be stored as 65). This has the advantage of being very simple and space efficient whilst providing enough (256) different values to represent all the symbols that might be provided on a typewriter.

The flaw in this scheme becomes obvious fairly quickly: there are far more than 256 different characters in use in all the writing systems of the world, especially when East Asian ideographic languages are considered. But, in the pre-internet days, this was not a big problem.

LiveCode, as a product first created before the rise of the internet, also adopted the 8-bit character sets of the platforms it ran on (which also meant that each platform used a different character set: MacRoman on Apple devices, CP1252 on Windows and ISO-8859-1 on Linux and Solaris). LiveCode terms these character encodings "native" encodings.

In order to overcome the limitations of 8-bit character sets, the Unicode Consortium was formed. This group aims to assign a unique numerical value ("codepoint") to each symbol used in every written language in use (and in a number that are no longer used!). Unfortunately, this means that a single byte cannot represent any possible character.

The solution to this is to use multiple bytes to encode Unicode characters and there are a number of schemes for doing so. Some of these schemes can be quite complex, requiring a varying number of bytes for each character, depending on its codepoint.

LiveCode previously added support for the UTF-16 encoding for text stored in fields but this could be cumbersome to manipulate as the variable-length aspects of it were not handled transparently and it could only be used in limited contexts. Unicode could not be used in control names, directly in scripts or in many other places where it might be useful.

In LiveCode 7.0, the engine has been extensively re-written to be able to handle Unicode text transparently throughout. The standard text manipulation operations work on Unicode text without any additional effort on your part; Unicode text can now be used to name controls, stacks and other objects; menus containing Unicode selections no longer require tags to be usable - anywhere text is used, Unicode should work.

Adding this support has required some changes but these should be minor. Existing apps should continue to run with no changes but some tweaking may be required in order to adapt them for full Unicode support - this is described in the next section - Creating Unicode Apps.

# Creating Unicode Apps

Creating stacks that support Unicode is no more difficult than creating any other stack but there are a few things that should be borne in mind when developing with Unicode. The most important of these is the difference between text and binary data - in previous versions of LiveCode, these could be used interchangeably; doing this with Unicode may not work as you expect (but it will continue to work for non-Unicode text).

When text is treated as binary data (i.e when it is written to a file, process, socket or other object outside of the LiveCode engine) it will lose its Unicode-ness: it will automatically be converted into the platform's 8-bit native character set and any Unicode characters that cannot be correctly represented will be converted into question mark '?' characters.

Similarly, treating binary data as text will interpret it as native text and won't support Unicode.

To avoid this loss of data, text should be explicitly encoded into binary data and decoded from binary data at these boundaries - this is done using the **textEncode** and **textDecode** functions (or its equivalents, such as opening a file using a specific encoding). 

Unfortunately, the correct text encoding depends on the other programs that will be processing your data and cannot be automatically detected by the LiveCode engine. If in doubt, UTF-8 is often a good choice as it is widely supported by a number of text processing tools and is sometimes considered to be the "default" Unicode encoding.

## New & Existing apps - things to look out for

* When dealing with binary data, you should use the **byte** chunk expression rather than **char** - **char** is intended for use with textual data and represents a single graphical character rather than an 8-bit unit.
* Try to avoid hard-coding assumptions based on your native language - the formatting of numbers or the correct direction for text layout, for example. LiveCode provides utilities to assist you with this.
* Regardless of visual direction, text in LiveCode is always in logical order - word 1 is always the first word; it does not depend on whether it appears at the left or the right.
* Even English text can contain Unicode characters - curly quotation marks, long and short dashes, accents on loanwords, currency symbols...

# New Commands, Functions & Syntax

## Chunk expressions: byte, char, codepoint, codeunit

**byte** *x* **to** *y* **of** *text*		-- Returns bytes from a binary string
**char** *x* **to** *y* **of** *text*			-- As a series of graphical units
**codepoint** *x* **to** *y* **of** *text*	-- As a series of Unicode codepoints
**codeunit** *x* **to** *y* **of** *text*		-- As a series of encoded units

A variety of new chunk types have been added to the LiveCode syntax to support the various methods of referring to the components of text. This set is only important to those implementing low-level functions and can be safely ignored by the majority of users.

The key change is that **byte** and **char** are no longer synonyms - a byte is strictly an 8-bit unit and can only be reliably used with binary data. For backwards compatibility, it returns the corresponding native character from Unicode text (or a '?' if not representable) but this behaviour is deprecated and should not be used in new code.

The **char** chunk type no longer means an 8-bit unit but instead refers to what would naturally be thought of as a single graphical character (even if it is composed of multiple sub-units, as in some accented text or Korean ideographs). Because of this change, it is inappropriate to use this type of chunk expression on binary data.

The **codepoint** chunk type allows access to the sequence of Unicode codepoints which make up the string. This allows direct access to the components that make up a character. For example, &aacute; can be encoded as (a,combining-acute-accent) so it is one character, but two codepoints (the two codepoints being a and combining-acute-accent).

The **codeunit** chunk type allows direct access to the UTF-16 code-units which notionally make up the internal storage of strings. The codeunit and codepoint chunk are the same if a string only contains unicode codepoints from the Basic Multilingual Plane. If, however, the string contains unicode codepoints from the Supplementary Planes, then such codepoints are represented as two codeunits (via the surrogate pair mechanism). The most important feature of the 'codeunit' chunk is that it guarantees constant time indexed access into a string (just as char did in previous engines) however it is not of general utility and should be reserved for use in scripts which need greater speed but do not need to process Supplmentary Plane characters, or are able to do such processing themselves.

The hierarchy of these new and altered chunk types is as follows: **byte** *w* of **codeunit** *x* of **codepoint** *y* of **char** *z* of **word**...

## Chunk expressions: paragraph, sentence and trueWord

The **sentence** and **trueWord** chunk expressions have been added to facilitate the processing of text, taking into account the different character sets and conventions used by various languages. They use the ICU library, which uses a large database of rules for its boundary analysis, to determine sentence and word breaks. ICU word breaks delimit not only whitespace but also individual punctuation characters; as a result the LiveCode **trueWord** chunk disregards any such substrings that contain no alphabetic or numeric characters.

The **paragraph** chunk is identical to the existing **line** chunk, except that it is also delimited by the Unicode paragraph separator (0x2029), which reflects paragraph breaking in LiveCode fields.

The hierarchy of these new chunk types is as follows: **trueword** *v* of **word** *w* of **item** *x* of **sentence** *y* of **paragraph** *z* of **line**...

## Synonym: segment

The **segment** chunk type has been added as a synonym to the existing **word** chunk. This in order to allow you to update your scripts to use the newer syntax in anticipation of a future change to make the behaviour of the **word** chunk match the new **trueWord** behaviour.

We would anticipate changing the meaning of **word** with our 'Open Language' project. It requires us to create a highly accurate script translation system to allow old scripts to be rewritten in new revised and cleaner syntax. It is at this point we can seriously think about changing the meaning of existing tokens, including **word**. Existing scripts will continue to run using the existing parser, and they can be converted (by the user) over time to use the newer syntax.

## Property: the formSensitive

set the **formSensitive** to false			-- Default value

This property is similar to the **caseSensitive** property in its behaviour - it controls how text with minor differences is treated in comparison operations.

Normalization is a process defined by the Unicode standard for removing minor encoding differences for a small set of characters and is more fully described in the **normalizeText** function.

## Command: open file/process/socket ... for <encoding> text

**open file** *"log.txt"* **for utf-8 text read**		-- Opens a file as UTF-8

Opens a file, process or socket for text I/O using the specified encoding. The encodings supported by this command are the same as those for the **textEncode** / **textDecode** functions. All text written to or read from the object will undergo the appropriate encoding/decoding operation automatically.

## Functions: textEncode, textDecode

**textEncode**(*string*, *encoding*)		-- Converts from text to binary data
**textDecode**(*binary*, *encoding*)		-- Converts from binary data to text

Supported encodings are (currently):
*"ASCII"
*"ISO-8859-1"		(Linux only)
*"MacRoman"		(OSX only)
*"Native"		(ISO-8859-1 on Linux, MacRoman on OSX, CP1252 Windows)
*"UTF-16"
*"UTF-16BE"
*"UTF-16LE"
*"UTF-32"
*"UTF-32BE"
*"UTF-32LE"
*"UTF-8"
*"CP1252"		(Windows only)

Spelling variations are ignored when matching encoding strings (i.e all characters other than [a-zA-z0-9] are ignored in matches as are case differences).

It is very highly recommended that any time you interface with things outside LiveCode (files, network sockets, processes, etc) that you explicitly **textEncode** any text you send outside LiveCode and **textDecode** all text received into LiveCode. If this doesn't happen, a platform-dependent encoding will be used (which normally does not support Unicode text).

It is not, in general, possible to reliably auto-detect text encodings so please check the documentation for the programme you are communicating with to find out what it expects. If in doubt, try "UTF-8".

## Functions: numToCodepoint, codepointToNum

**numToCodepoint**(*number*)			-- Converts a Unicode codepoint to text
**codepointToNum**(*codepoint*)		-- Converts a codepoint to an integer


These functions convert between the textual form of a Unicode character and its numerical identifier ("codepoint"). Codepoints are integers in the range 0x000000 to 0x10FFFF that identify Unicode characters. For example, the space (" ") character is 0x20 and "A" is 0x41.

The codepointToNum function raises an exception if the argument contains multiple codepoints; it should generally be used in the form:

`	codepointToNum(codepoint x of string)`

The numToCodepoint function raises an exception if the given integer is out of range for Unicode codepoints (i.e if it is negative or if it is greater than 0x10FFFF). Codepoints that are not currently assigned to characters by the latest Unicode standard are not considered to be invalid in order to ensure compatibility with future standards.

## Functions: numToNativeChar, nativeCharToNum

**numToNativeChar**(*number*)			-- Converts an 8-bit value to text
**nativeCharToNum**(*character*)			-- Converts a character to an 8-bit value


These functions convert between text and native characters and are replacements for the deprecated **numToChar** and **charToNum** functions.

As the "native" character sets for each platform have a limited and different repertoire, these functions should not be used when preservation of Unicode text is desired. Any characters that cannot be mapped to the native character set are replaced with a question mark character ('?').

Unless needed for compatibility reasons, it is recommended that you use the **numToCodepoint** and **codepointToNum** functions instead.

## Function: normalizeText

**normalizeText**(*text*, *normalForm*)		-- Normalizes to the given form

The **normalizeText** function converts a text string into a specific 'normal form'.

Use the **normalizeText** function when you require a specific normal form of text.

In Unicode text, the same visual string can be represented by different character sequences. A prime example of this is precomposed characters and decomposed characters: an 'e' followed by a combining acute character is visually indistinguishable from a precombined '&eacute;' character. Because of the confusion that can result, Unicode defined a number of "normal forms" that ensure that character representations are consistent.

The normal forms supported by this function are:
*"NFC"		- precomposed
*"NFD"		- decomposed
*"NFKC"	- compatibility precomposed
*"NFKD"	- compatibility decomposed

The "compatibility" normal forms are designed by the Unicode Consortium for dealing with certain legacy encodings and are not generally useful otherwise.

It should be noted that normalization does not avoid all problems with visually-identical characters; Unicode contains a number of characters that will (in the majority of fonts) be indistinguishable but are nonetheless completely different characters (a prime example of this is "M" and U+2164 "&#8559;" ROMAN NUMERAL ONE THOUSAND).

Unless the **formSensitive** handler property is set to true, LiveCode ignores text normalization when performing comparisons (is, <>, etc).

Returns: the text normalized into the given form.

`	set the formSensitive to true
	put "e" & numToCodepoint("0x301") into tExample		-- Acute accent
	put tExample is "&eacute;"						-- Returns false
	put normalizeText(tExample, "NFC") is "&eacute;"		-- Returns true`


## Function: codepointProperty

**codepointProperty**("A", "Script")	-- "Latin"
**codepointProperty**("&beta;", "Uppercase")	-- false
**codepointProperty**("&sigma;", "Name")		-- GREEK SMALL LETTER SIGMA

Retrieves a UCD character property of a Unicode codepoint.

The Unicode standard and the associated Unicode Character Database (UCD) define a series of properties for each codepoint in the Unicode standard. A number of these properties are used internally by the engine during text processing but it is also possible to query these properties directly using this function.

This function is not intended for general-purpose use; please use functions such as toUpper or the "is" operators instead.

There are many properties available; please see the version 6.3.0 of the Unicode standard, Chapter 4 and Section 5 of Unicode Technical Report (TR)#44 for details on the names and values of properties. Property names may be specified with either spaces or underscores and are not case-sensitive.

Examples of supported properties are:

*"Name"			- Unique name for this codepoint
*"Numeric_Value"		- Numerical value, e.g. 4 for "4"
*"Quotation_Mark"		- True if the codepoint is a quotation mark
*"Uppercase_Mapping"	- Uppercase equivalent of the character
*"Lowercase"			- True if the codepoint is lower-case

# Updated Functions

# Function: binaryEncode

A new letter has been introduced to allow one to binary encode unicode strings.
Following the dictionary definitions, it consists of:

u{<encoding>}: convert the input string to the encoding specified in the curly braces, and output up to amount bytes of the string created - stopping at the last encoded character fitting in the amount - padding with '\0'.

U{<encoding>}: convert the input string to the encoding specified in the curly braces, and output up to amount bytes of the string created - stopping at the last encoded character fitting in the amount - padding with encoded spaces, and then '\0' if the last encoded space cannot fit within the amount specified.

The encoding, surrounded by curly braces, is optional - no one specified would default to the behaviour of 'a' - and must match one of those applicable to textEncode

# Function: binaryDecode

A new letter has been introduced to allow one to binary decode unicode strings.
Following the dictionary definitions, it consists of:

u{<encoding>}: convert amount bytes of the input string to the specified encoding, padding with '\0'.

U{<encoding>}: converts amount bytes of the input to the specified encoding, skipping trailing spaces.

The encoding, surrounded by curly braces, is optional - no one specified would default to the behaviour of 'a' - and must match one of those applicable to textEncode

# Deprecated Features

## Functions: numToChar, charToNum

These functions should not be used in new code as they cannot correctly handle Unicode text.

## Property: useUnicode

This property should not be used in new code, as it only affects the behaviour of **numToChar** and **charToNum**, which are themselves deprecated.

## Functions: uniEncode, uniDecode

These functions should not be used in new code as their existing behaviour is incompatible with the new, transparent Unicode handling (the resulting value will be treated as binary data rather than text). These functions are only useful in combination with the also-deprecated unicode properties described below.

## Function: measureUnicodeText

This function should not be used in new code. **measureUnicodeText**(*tText*) is equivalent to **measureText**(**textDecode**(*tText*, "UTF16")).

## Properties: unicodeText, unicodeLabel, unicodeTitle, unicodeTooltip, unicodePlainText, unicodeFormattedText

These properties should not be used in new code; simply set the text, label, title etc. as normal. Assigning values other than those returned from **uniEncode** to these properties will not produce the desired results.

The following are now equivalent:

`	set the unicodeText of field 1 to tText
	set the text of field 1 to textDecode(tText, "UTF16")` 
and similarly for the other unicode-prefixed properties.



