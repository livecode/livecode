#!/bin/bash
#
# This script is used to provide a bridge for old LiveCode auto-updaters that
# expect the LiveCode update DMG to contain an installer rather than the app
# bundle that is now is. It fires off an AppleScript that opens a Finder
# window on the DMG so that the user can drag the app bundle to their
# Applications folder.

# Location of this script file
# <something>.app/Contents/MacOS/installer
self=${BASH_SOURCE[0]}
selfdir=$(dirname "${self}")

# Location of the AppleScript
# <something>.app/Contents/MacOS/installer.scpt
script="${selfdir}"/installer.scpt

# Directory to open in Finder
# AppleScript can only deal with absolute paths and doesn't like '..'
target=$(cd "${selfdir}/../../.." && pwd)

osascript "${script}" "${target}"
exit $?
