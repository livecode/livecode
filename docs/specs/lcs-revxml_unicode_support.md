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

The effect of transparent Unicode support on any API depends on classifying all
parameters and return values in one of the following ways:

  - always text: the value is presumed to be a text string by the API and currently
processed as such (i.e. as a native encoded string).
  - always binary: the value is presumed to be data by the API and is never
interpreted as text (i.e. a block of bytes)
  - maybe text: the value is actually text data but the API has relied on explicit
code in script to translate it to the native encoding (i.e. utf-8 bytes) as required.

If a value is 'always text' then there is no problem with updating the parameter
or return value to be treated as a string in the revised V7 interface.

If a value is 'always binary' then there is no problem with updating the parameter
or return value to be treated as data in the revised V7 interface.

If an API (as a whole) only has 'always text' and 'always binary' values then it can
be updated in a completely backwards-compatible manner without explicit changes to
the script-visible API.

Unfortunately though, if an API has 'maybe text' values then explicit consideration
needs to be given to three use-cases:

  - existing: existing code which uses the APIs already
  - transitional: code which is in transition from current usage to updated usage
  - new: new code which only needs updated usage

The existing case means that any code which use the APIs right now must
function exactly the same after the changes as before. In practice this means that
the 'maybe text' values in the API must remain as binary data in the default case
of the author not having made any changes to their scripts.

The new case is where a new app is being developed from scratch. In this case
the author would not want to be bothered by any backwards-compatibility
requirements. In practice this means that there must be a mode of using the 
revised API where the 'maybe text' values in the API are eliminated.

The transitional case is where an author is in the process of updating their
scripts to be Unicode-aware. This is particularly important in the situation where
an API creates a state object which might be manipulated anywhere in a large
program and where parameters or results of the APIs which manipulate it which
are binary data *might* be text strings (as in this case there will be explicit
code in script for 'doing the right thing' with the binary data). In practice this
means that there must be a mode of using the revised API where the author can update
some code to remove explicit handling of 'maybe text' values; whilst code which still
has the explicit handling works the same as it did before.

## revXML Specific Considerations

In the case of revXML all parameters and results fall into three categories:

  - an id
  - a path
  - options
  - data that is passed to, or passed back from, libxml2

Here we can assume that any parameter or result which is 'id','path' or 'options'
is 'text'. However, as libxml2 uses utf-8 as the universal internal encoding (it
translates to/from the encoding specified in the xml file automatically) we have
to assume that any use of data passed directly to/from libxml2 through the revXML
APIs is 'maybe text'.

As revXML is a state-object based API then there is an object to which the intent
of use of the object (with regards whether 'maybe text' values are treated as
binary or text in script) can be attached.

In practice, this means we can achieve usage in all three possible use-cases by
having duplicate entry points for each API - the existing set, and a set with
'Unicode' as a suffix.

This would work as follows:

  - the existing Create APIs would create a document object which was marked as
treating 'maybe text' values as binary.
  - the new CreateUnicode APIs would create a document object which was marked as
treating 'maybe text' values as text.
  - the existing non-Unicode APIs would treat 'maybe text' values as binary for
document objects marked as binary
  - the existing non-Unicode APIs would treat 'maybe text' values as text for
document objects marked as text
  - the new non-Create Unicode APIs would treat 'maybe text' values as text

This covers the three required use-cases as follows:

  - existing: script will be using non-Unicode Create API, so will continue to
create document objects on which all the existing APIs work the same way as they
do now.
  - transitional: the author can update all uses of the non-Create non-Unicode revXML
APIs to the unicode variants as they update their code.
  - new: the author uses the Unicode Create API, and can then use the non-Unicode revXML
APIs to manipulate the created documents.



