#!/bin/bash

set -x

# If this script is not run as root, the DMG will have incorrect ownership
if [ $EUID -ne 0 ] ; then
	# If this is being invoked with the intention of being privileged,
	# this failure is a hard error.
	if [ "${GENERATE_DMG_SCRIPT}" != "" ] ; then
		echo >&2 "ERROR: this script must be run as root"
		exit -1
	fi	
	echo >&2 "WARNING: not building DMG as root; app will have incorrect ownership"
fi

# Arguments are the name of the volume to create, the input folder and the output filename
if [ $# -ne 3 ] ; then
	echo >&2 "ERROR: usage: ./generate-dmg.sh volname srcfolder output"
	exit -2
fi

volname="$1"
srcfolder="$2"
output="$3"

# Make the DMG
# UID and GID 99 are magical - they are needed to ensure ownership is correct
# when the app bundle is copied out of the DMG. They are also the reason that
# this script needs to be run with root permissions.
#
if [ $EUID -eq 0 ] ; then
  ids="-uid 99 -gid 99"
fi

hdiutil create -fs HFS+ -format UDRW -scrub ${ids} -attach -volname "${volname}" -srcfolder "${srcfolder}" "${output}"

# Ensure the ownership of the output image is correct
user=$(who am i | awk '{print $1}')
group=$(id -g -n "${user}")
chown ${user}:${group} "${output}"
chmod 644 "${output}"
