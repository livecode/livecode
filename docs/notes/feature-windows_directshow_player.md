# Windows DirectShow Player Control

Due to the recent decision by Apple to end support for QuickTime on Windows, the player implementation on that platform has been replaced with one based on DirectShow - a Multimedia API available on all supported Windows versions.
The new implementation should function as a drop-in replacement for the old one, though some properties are not yet implemented.

## Property Changes
Functionality of the following Player properties on Windows has changed:

**Removed:** alwaysBuffer, callbacks, enabledTracks, mediaTypes, mirrored, trackCount, tracks.

**Added:** loadedTime.

We will be working to reinstate these properties where possible.

In addition, the following properties specific to QuickTime and QTVR have been deprecated:

**Deprecated:** constraints, currentNode, movieControllerId, nodes, pan, tilt, zoom

## Supported File Formats

Media format support in DirectShow depends on the installed codecs, though the following page lists the file formats and compression types available as standard: https://msdn.microsoft.com/en-us/library/ms787745(VS.85).aspx