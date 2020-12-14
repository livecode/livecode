# Windows MediaFoundation Player Control

Due to the recent decision by Apple to end support for QuickTime on
Windows, the player implementation on that platform has been replaced
with one based on DirectShow.  This is a multimedia API that is
available by default on all versions of Windows supported by LiveCode.

The new implementation should function as a drop-in replacement for
the old one, though some properties are not yet implemented.

### Property Changes
On Windows, the behaviour of some properties of the player control have changed.

- The **tracks**, **trackCount**, and **enabledTracks** previousy did not work
  on Windows but now do.
- The **loadedTime** property no longer works on Windows, but will be re-enabled
  if possible in a subsequent release.

### Supported File Formats

Media format support in the new Windows player control depends on
which codecs are installed.

A list of the
[file formats and compression types available as standard](https://docs.microsoft.com/en-us/windows/win32/medfound/supported-media-formats-in-media-foundation)
on Windows is available in the Windows Dev Center documentation
