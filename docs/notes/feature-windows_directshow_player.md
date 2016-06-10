---
version: 8.1.0-dp-1
---
# Windows DirectShow Player Control

Due to the recent decision by Apple to end support for QuickTime on
Windows, the player implementation on that platform has been replaced
with one based on DirectShow.  This is a multimedia API that is
available by default on all versions of Windows supported by LiveCode.

The new implementation should function as a drop-in replacement for
the old one, though some properties are not yet implemented.

### Property Changes
On Windows, the behaviour of some properties of the player control have changed.

- The **loadedTime** property previously did not work on Windows, but now does.
- The **alwaysBuffer**, **enabledTracks**,
  **mediaTypes**, **mirrored**, **trackCount** and **tracks**
  properties do not currently work, but will be re-enabled in a
  subsequent release.

On all platforms, the following player control properties, which are
specific to QuickTime and QTVR, have been deprecated: **constraints**,
**currentNode**, **movieControllerId**, **nodes**, **pan**, **tilt**,
and **zoom**.

### Supported File Formats

Media format support in the new Windows player control depends on
which codecs are installed.

A list of the
[file formats and compression types available as standard](https://msdn.microsoft.com/en-us/library/ms787745(VS.85).aspx)
on Windows is available in the MSDN documentation
