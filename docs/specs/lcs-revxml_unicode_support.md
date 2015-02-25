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

## General Considerations

The effect of transparent Unicode support on any API depends on whether any
particular parameter or result is treated as binary data or as a text string.
Which of these this is in any particular case depends on how the API is
using the parameter or result (prior to V7 nothing distinguished between these
two things as text and binary data were 'the same thing).

Additionally, any updates to the API need to take into account the following
use-cases:

  - existing: existing code which uses the APIs already
  - transitional: code which is in transition from current usage to updated usage
  - new: new code which only needs updated usage

The existing case means that any code which use the APIs right now must
function exactly the same after the changes as before. In practice this means
parameters or results of API calls which are treated as text strings (in 
script) can be updated to Unicode directly; whereas parameters or results
which are binary data which *might* be text strings cannot.

The transitional case is where an author is in the process of updating their
scripts to be Unicode-aware. This is particularly important in the case where
an API creates a state object which might be manipulated anywhere in a large
program and where parameters or results of the APIs which manipulate it which
are binary data *might* be text strings (as in this case there will be explicit
code in script for 'doing the right thing' with the binary data).

The new case is where a new app is being developed from scratch. In this case
the author would not want to be bothered by any backwards-compatibility
requirements.

## revXML Specific Considerations

In the case of revXML all parameters and results fall into two categories:

    - a path
    - options
    - data that is passed to, or passed back from, libxml2

Here we can assume that any parameter or result which is a 'path' or 'options'
is being treated as a text string in any existing script (as that is how the
revXML external processes it).

As libxml2 uses utf-8 as the universal internal encoding (it translates to/from
the encoding specified in the xml file automatically) we have to assume that
any use of data passed directly to/from libxml2 through the revXML APIs is
being treated as binary data in script (and thus being encoded/decoded as
needed).





