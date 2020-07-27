#!/bin/python

# Update these lists if you need different SDK versions!
iphoneos_versions = ["13.5", "13.2", "12.1", "11.2", "10.2"]
iphonesimulator_versions = ["13.5", "13.2", "12.1", "11.2", "10.2"]
macosx_versions = ["10.9"]


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

import os
import os.path
import re
import subprocess
import sys
from distutils.version import LooseVersion

def min_sdk(versions):
    if len(versions) == 0:
        return None
    return sorted(versions, key=LooseVersion)[0]

def sdk_path(xcode_app, platform, version):
    tmpl = "{0}/Contents/Developer/Platforms/{1}.platform/Developer/SDKs/{1}{2}.sdk"
    return os.path.abspath(tmpl.format(xcode_app, platform, version))

def plist_path(xcode_app, platform):
    tmpl = "{}/Contents/Developer/Platforms/{}.platform/Info.plist"
    return os.path.abspath(tmpl.format(xcode_app, platform))

def check_sdk_path(xcode_app, platform, version):
    path = sdk_path(xcode_app, platform, version)
    if os.path.exists(path) and os.path.isdir(path):
        return path
    else:
        return None

def is_sdk_present(xcode_app, platform, version):
    return check_sdk_path(xcode_app, platform, version) is not None

def xcode_paths(base_dir):
    return_list = []
    level_0 = os.listdir(base_dir)
    for entry in level_0:
        if(re.match(r"Xcode.*\.app", entry)):
            return_list.append(entry)
        if(not os.path.isdir(entry)):
            continue
        level_1 = os.listdir(entry)
        for l1_entry in level_1:
            if(re.match(r"Xcode.*\.app", l1_entry)):
                return_list.append(entry + "/" + l1_entry)
    return return_list

def target_xcode(base_dir, beta):
    if beta:
        return "{}/{}".format(base_dir, "Xcode-beta.app")
    else:
        return "{}/{}".format(base_dir, "Xcode.app")

class SDKInstaller(object):
    def __init__(self, base_dir):
        self._base_dir = base_dir
        self._success = True

    def _diagnostic(self, message):
        print("# " + message)

    def _status(self, state, message):
        if state:
            print("ok - " + message)
        else:
            print("not ok - " + message)
            self._success = False

    def install(self, platform, versions, beta):
        target = target_xcode(self._base_dir, beta)
        self._install_sdks(target, platform, versions)
        self._set_sdk_minversion(target, platform, versions)

    def is_successful(self):
        return self._success

    def _set_sdk_minversion(self, xcode_app, platform, versions):
        self._diagnostic("Setting {} SDK minimum version".format(platform))
        version = min_sdk(versions)
        path = plist_path(xcode_app, platform)
        cmd = ["/usr/bin/defaults", "write", path, "MinimumSDKVersion",
               "'{}'".format(version)]
        success = (subprocess.call(cmd) == 0)
        self._status(success, "{} {}".format(platform, version))

    def _find_sdk(self, platform, version):
        for xcode_app in xcode_paths(self._base_dir):
            path = check_sdk_path(xcode_app, platform, version)
            if path is not None:
                return os.path.realpath(path)
        return None

    def _install_sdk(self, xcode_app, platform, version):
        desc = "{} {}".format(platform, version)
        if is_sdk_present(xcode_app, platform, version):
            self._status(True, desc)
            return

        target_sdk = self._find_sdk(platform, version)
        if target_sdk is None:
            self._status(False, desc)
            return

        install_path = sdk_path(xcode_app, platform, version)
        try:
            os.symlink(target_sdk, install_path)
            self._status(True, desc)
        except OSError as e:
            self._diagnostic("{} failed: {}".format(install_path, e))
            self._status(False, desc)

    def _install_sdks(self, xcode_app, platform, versions):
        self._diagnostic("Installing {} SDK symlinks".format(platform))
        for version in versions:
            self._install_sdk(xcode_app, platform, version)

class CachingSDKInstaller(SDKInstaller):
    def __init__(self, base_dir, cache_dir):
        super(CachingSDKInstaller, self).__init__(base_dir)
        self._cache_dir = cache_dir

    def _cached_sdk_path(self, platform, version):
        tmpl = "{0}/{1}/{1}{2}.sdk"
        return os.path.abspath(tmpl.format(self._cache_dir, platform, version))

    def _check_cached_sdk_path(self, platform, version):
        path = self._cached_sdk_path(platform, version)
        if os.path.exists(path) and os.path.isdir(path):
            return path
        else:
            return None

    def _cache_sdk(self, platform, version):
        # Check cache
        cache = self._check_cached_sdk_path(platform, version)
        if cache is not None:
            return

        # Search app bundles
        original = super(CachingSDKInstaller, self)._find_sdk(platform, version)
        if original is None:
            return

        # Copy the original into cache
        # N.b. use `cp -a` in order to make sure all resource forks etc. are
        # preserved
        self._diagnostic("Caching {} {} SDK".format(platform, version))

        cache = self._cached_sdk_path(platform, version)

        platform_dir = os.path.dirname(cache)
        if not os.path.isdir(platform_dir):
            os.makedirs(platform_dir)

        cmd = ["/bin/cp", "-a", original, cache]
        subprocess.check_call(cmd)

    def _find_sdk(self, platform, version):
        # Any available SDKs should have already been cached
        return self._check_cached_sdk_path(platform, version)

    def _install_sdk(self, xcode_app, platform, version):
        self._cache_sdk(platform, version)
        super(CachingSDKInstaller, self)._install_sdk(xcode_app, platform, version)

if __name__ == "__main__":
    if "--cache" in sys.argv:
        installer = CachingSDKInstaller(".", "./XcodeSDKs")
    else:
        installer = SDKInstaller(".")

    beta = "--beta" in sys.argv

    installer.install("iPhoneOS", iphoneos_versions, beta)
    installer.install("iPhoneSimulator", iphonesimulator_versions, beta)
    installer.install("MacOSX", macosx_versions, beta)

    if not installer.is_successful():
        print("ERROR: Some SDKs couldn't be installed")
        sys.exit(1)
