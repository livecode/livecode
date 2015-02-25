# LiveCode Script - revXML Unicode Support
Copyright 2015 LiveCode Ltd.

## Introduction

The revXML external is a high-level wrapper around libxml2 providing the ability
to manipulate XML files from LiveCode script.

To use revXML, script must first create a document using one of the Create
functions. This binds an XML document to a unique integer id which can then be
used in the rest of the revXML API.

As it stands, revXML is based on the pre-V7 version 0 externals interface. This
means that both arguments to, and results from, its external functions are treated
as native encoded (binary) strings. In particular, it passes the data it receives
verbatim between libxml2 and the caller. As libxml2 uses utf-8 as its universal
text encoding this means that to use revXML you (essentially) need to textEncode and
textDecode all data coming in and out of it as UTF-8; or treat it as binary data.

In order to put revXML on the same footing with regards transparent Unicode support
as the updates to the main LiveCode syntax, it is necessary to update revXML so it
uses (in an appropriate way) the revisions to the V0 externals interface present
in V7 and above.
