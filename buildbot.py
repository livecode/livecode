#!/usr/bin/env python
# Copyright (C) 2017 LiveCode Ltd.
#
# This file is part of LiveCode.
#
# LiveCode is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3 as published by the Free
# Software Foundation.
#
# LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with LiveCode.  If not see <http://www.gnu.org/licenses/>.

# This file contains rules used by the LiveCode Buildbot installation
# at <https://vulcan.livecode.com/>
#
# Tasks that may be run on Windows workers must be implemented in this
# file.  Other tasks may be implemented in buildbot.mk, and this
# script redirects to that Makefile.  All buildbot operations should
# occur via this script, so that buildbot does not invoke
# platform-specific tools directly.

import sys
import subprocess
import os
import platform as _platform
import shutil
import tarfile

# The set of platforms for which this branch supports automated builds
BUILDBOT_PLATFORMS = ('linux-x86', 'linux-x86_64', 'android-armv6', 'mac',
    'ios', 'win-x86', 'emscripten')
# The set of build tasks that this branch supports
BUILDBOT_TARGETS = ('config', 'compile', 'bin-archive', 'bin-extract',
    'dist-notes', 'dist-docs', 'dist-server', 'dist-tools', 'dist-upload',
    'distmac-archive', 'distmac-extract', 'distmac-disk')

SKIP_EXIT_STATUS = 88

def usage(exit_status):
    print(
"""Perform continuous integration and release build steps.

Usage:
  buildbot.py TARGET

Environment variables:
  BUILD_EDITION      LiveCode edition name ("commercial" or "community")
  BUILD_PLATFORM     The target platform for build (e.g. "ios")
  BUILD_SUBPLATFORM  The target subplatform (e.g. "iphoneos10.2")

Many tasks are deferred to the "buildbot.mk" Makefile and the
"config.py" configuration script.  Refer to these for other
environment variables that may affect the build.
""")

    sys.exit(exit_status)

def error(message):
    print("ERROR: " + message)
    sys.exit(1)

def get_build_platform():
    platform = (os.environ.get('BUILD_PLATFORM'),
                os.environ.get('BUILD_SUBPLATFORM'))
    if platform[0] is None:
        error('You must set $BUILD_PLATFORM')
    return platform

def get_buildtype():
    return os.environ.get('BUILDTYPE', 'Debug')

def get_build_edition():
    return os.environ.get('BUILD_EDITION', 'community')

################################################################
# Defer to buildbot.mk
################################################################

def exec_buildbot_make(target):
    args = ["make", "-f", "buildbot.mk", target]
    print(' '.join(args))
    sys.exit(subprocess.call(args))

################################################################
# Configure with gyp
################################################################

def exec_configure(args):
    import config
    print('config.py ' + ' '.join(args))
    sys.exit(config.configure(args))

def do_configure():
    platform, subplatform = get_build_platform()

    if platform == 'ios':
        if subplatform is None:
            error('You must set $BUILD_SUBPLATFORM for iOS builds')
        exec_configure(['--platform', 'ios',
                        '--generator-output',
                        'build-{}-{}/livecode'.format(platform, subplatform),
                        '-Dtarget_sdk=' + subplatform])
    else:
        exec_configure(['--platform', platform])
    return 0

################################################################
# Compile
################################################################

def exec_make(target):
    args = ['make', target]
    print(' '.join(args))
    sys.exit(subprocess.call(args))

def exec_msbuild(platform):
    # Run the make.cmd batch script; it's run using Wine if this is
    # not actually a Windows system.
    cwd = 'build-' + platform

    if _platform.system() == 'Windows':
        args = ['cmd', '/C', '..\\make.cmd']
        print(' '.join(args))
        sys.exit(subprocess.call(args, cwd=cwd))

    else:
        args = ['wine', 'cmd', '/K', '..\\make.cmd']
        print(' '.join(args))
        exit_status = sys.exit(subprocess.call(args, cwd=cwd))

        # Clean up any Wine processes that are still hanging around.
        # This is important in case the build fails.
        args = ['wineserver', '-k', '-w']
        subprocess.call(args, cwd=cwd)

        sys.exit(exit_status)

def do_compile():
    platform, subplatform = get_build_platform()
    if platform.startswith('win-'):
        return exec_msbuild(platform)
    else:
        # Just defer to the top level Makefile
        if platform == 'ios':
            if subplatform is None:
                error('You must set $BUILD_SUBPLATFORM for iOS builds')
            target = 'compile-{}-{}'.format(platform, subplatform)
        else:
            target = 'compile-' + platform
        return exec_make(target)

################################################################
# Archive / extract built binaries
################################################################

def do_bin_archive():
    platform, subplatform = get_build_platform()
    bindir = platform + '-bin'
    shutil.make_archive(bindir, 'bztar', '.', bindir)

################################################################
# Main entry point
################################################################

def buildbot_task(target):
    # Check that this branch can actually be built for the specified platform
    platform, subplatform = get_build_platform()
    if not platform in BUILDBOT_PLATFORMS:
        print('Buildbot build for "{}" platform is not supported'.format(platform))
        sys.exit(SKIP_EXIT_STATUS)

    # Check that this branch supports performing the requested buildbot task
    if not target in BUILDBOT_TARGETS:
        print('Buildbot build step "{}" is not supported'.format(target))
        sys.exit(SKIP_EXIT_STATUS)

    if target == 'config':
        return do_configure()
    elif target == 'compile':
        return do_compile()
    elif target == 'bin-archive':
        return do_bin_archive()
    else:
        return exec_buildbot_make(target)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        error("You must specify a buildbot target stage")
    sys.exit(buildbot_task(sys.argv[1]))
