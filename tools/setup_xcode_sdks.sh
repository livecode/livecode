#!/bin/bash

# Update these lists if you need different SDK versions!

iphoneos_versions="9.3 9.2 8.2"
iphonesimulator_versions="9.3 9.2 8.2 7.1 6.1 5.1"
macosx_versions="10.8 10.6"

# This tool creates the symlinks required for Xcode builds of LiveCode.
#
# There are a few steps you need to take before you can run this tool:
#
# 1) Create a suitable directory ${dir} to hold your Xcode apps
#
# 2) Download and extract the various versions of Xcode that you need.  You
#    should install them to ${dir}/Xcode_${ver}.app.  For example, Xcode 6.2
#    should be installed as ${dir}/Xcode_6_2.app
#
# 3) Download and install any extra SDKs that you need, by loading each version
#    of Xcode and using the Xcode -> Preferences -> Downloads window
#
# 3) Make ${dir}/Xcode.app a symlink to the newest version of Xcode available.
#    For example, you could run:
#
#        cd ${dir}
#        ln -s Xcode_6_3.app Xcode.app
#
# 4) Update the lists of SDK versions (above)
#
# 5) Run this script:
#
#        cd ${dir}
#        sh /path/to/setup_xcode_sdks.sh
#
# 6) Open Xcode!
#
#        open ${dir}/Xcode.app

success="yes"

# sdk_path XCODE PLATFORM VERSION
sdk_path() {
  echo "$PWD/$1/Contents/Developer/Platforms/$2.platform/Developer/SDKs/$2$3.sdk"
}

# plist_path XCODE PLATFORM
plist_path() {
  echo "$PWD/$1/Contents/Developer/Platforms/$2.platform/Info.plist"
}

# have_sdk XCODE PLATFORM VERSION
have_sdk () {
  local p=$(sdk_path $1 $2 $3)
  test -a "$p" -a -d "$p/"
}

# min_sdk VERSION_LIST
min_sdk () {
  echo "$1" | xargs -n1 | sort -u | head -n1
}

# find_sdk PLATFORM VERSION
find_sdk () {
  for xcode in Xcode_*.app; do
    if have_sdk $xcode $1 $2; then
      sdk_path $xcode $1 $2
      return
    fi
  done
}

# set_sdk_minversion PLATFORM VERSION_LIST
set_sdk_minversion () {
  echo "# Setting $1 SDK minimum version"
  local v=$(min_sdk "$2")
  local plist="$(plist_path Xcode.app $1)"
  if defaults write "${plist}" MinimumSDKVersion "'${v}'"; then
    echo "ok - $1 $v"
  else
    echo "not ok - $1 $v"
    success="no"
  fi
}

# install_sdks PLATFORM VERSION_LIST
install_sdks () {
  echo "# Installing $1 SDK symlinks"
  for v in $2; do
    if have_sdk Xcode.app $1 $v; then
      # SDK is already set up
      echo "ok - $1 $v"

    else
      # Try to link the required SDK into place
      local target_sdk=$(find_sdk $1 $v)

      if [ -n "$target_sdk" ] &&
        ln -s "$target_sdk" $(sdk_path Xcode.app $1 $v); then
        echo "ok - $1 $v"
      else
        echo "not ok - $1 $v"
        success="no"
      fi
    fi
  done

  set_sdk_minversion "$1" "$2"
}

install_sdks "iPhoneOS" "$iphoneos_versions"
install_sdks "iPhoneSimulator" "$iphonesimulator_versions" &&
install_sdks "MacOSX" "$macosx_versions"

if [ $success != "yes" ]; then
  echo >&2
  echo "ERROR: Some SDKs couldn't be installed" >&2
  exit 1
fi
