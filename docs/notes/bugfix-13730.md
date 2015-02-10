# Cannot specify text encoding of server script file.
You can now specify the text encoding of a server script file - that is a file that begins with #! and is passed to LiveCode Server.

To specify the text encoding of a server script file, the second line of the file must be a comment and contain the string coding=<encoding> or coding:<encoding>.

For example, to specify a script file as being encoded by UTF-8 one could use:

#! /usr/bin/lcserver
# -*- coding: utf-8 -*-

The interpretation of the second line comment is the same as that used by Python - the engine searches for a match to the regular expression coding[=:]\s*([-\w.]+) and then interprets the encoding part in the standard way (stripping '-' and normalizing to uppercase).

