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
import uuid

# LiveCode build configuration script
import config
import fetch

# The set of build tasks that this branch supports
BUILDBOT_TARGETS = ('fetch', 'config', 'compile', 'bin-archive', 'bin-extract',
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

def get_target_triple():
    return os.environ.get('BUILD_TARGET_TRIPLE')

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

def check_target_triple():
    # Check that this branch can actually be built for the specified platform
    triple = get_target_triple()
    if not triple in config.BUILDBOT_PLATFORM_TRIPLES:
        print('Buildbot build for "{}" platform is not supported'.format(triple))
        sys.exit(SKIP_EXIT_STATUS)

################################################################
# Defer to buildbot.mk
################################################################

def exec_buildbot_make(target):
    args = ["make", "-f", "buildbot.mk", target]
    print(' '.join(args))
    sys.exit(subprocess.call(args))

################################################################
# Fetch prebuilts
################################################################

def exec_fetch(args):
    print('fetch.py ' + ' '.join(args))
    sys.exit(fetch.fetch(args))

def do_fetch():
    check_target_triple()
    exec_fetch(['--target', get_target_triple()])

################################################################
# Configure with gyp
################################################################

def exec_configure(args):
    print('config.py ' + ' '.join(args))
    sys.exit(config.configure(args))

def do_configure():
    check_target_triple()
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

# mspdbsrv is the service used by Visual Studio to collect debug
# data during compilation.  One instance is shared by all C++
# compiler instances and threads.  It poses a unique challenge in
# several ways:
#
# - If not running when the build job starts, the build job will
#   automatically spawn it as soon as it needs to emit debug symbols.
#   There's no way to prevent this from happening.
#
# - The build job _doesn't_ automatically clean it up when it finishes
#
# - By default, mspdbsrv inherits its parent process' file handles,
#   including (unfortunately) some log handles owned by Buildbot.  This
#   can prevent Buildbot from detecting that the compile job is finished
#
# - If a compile job starts and detects an instance of mspdbsrv already
#   running, by default it will reuse it.  So, if you have a compile
#   job A running, and start a second job B, job B will use job A's
#   instance of mspdbsrv.  If you kill mspdbsrv when job A finishes,
#   job B will die horribly.  To make matters worse, the version of
#   mspdbsrv should match the version of Visual Studio being used.
#
# This class works around these problems:
#
# - It sets the _MSPDBSRV_ENDPOINT_ to a value that's probably unique to
#   the build, to prevent other builds on the same machine from sharing
#   the same mspdbsrv endpoint
#
# - It launches mspdbsrv with _all_ file handles closed, so that it
#   can't block the build from being detected as finished.
#
# - It explicitly kills mspdbsrv after the build job has finished.
#
# - It wraps all of this into a context manager, so mspdbsrv gets killed
#   even if a Python exception causes a non-local exit.
class UniqueMspdbsrv(object):
    def __enter__(self):
        os.environ['_MSPDBSRV_ENDPOINT_'] = str(uuid.uuid4())

        mspdbsrv_exe = os.path.join(config.get_program_files_x86(),
            'Microsoft Visual Studio\\2017\\BuildTools\\VC\\Tools\\MSVC\\14.10.25017\\bin\\HostX86\\x86\\mspdbsrv.exe')
        args = [mspdbsrv_exe, '-start', '-shutdowntime', '-1']
        print(' '.join(args))
        self.proc = subprocess.Popen(args, close_fds=True)
        return self

    def __exit__(self, type, value, traceback):
        self.proc.terminate()
        return False

def exec_msbuild(platform):
    # Run the make.cmd batch script; it's run using Wine if this is
    # not actually a Windows system.
    cwd = 'build-' + platform

    if _platform.system() == 'Windows':
        with UniqueMspdbsrv() as mspdbsrv:
            args = ['cmd', '/C', '..\\make.cmd']
            print(' '.join(args))
            result = subprocess.call(args, cwd=cwd)

        sys.exit(result)

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
    check_target_triple()

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
    # Check that this branch supports performing the requested buildbot task
    if not target in BUILDBOT_TARGETS:
        print('Buildbot build step "{}" is not supported'.format(target))
        sys.exit(SKIP_EXIT_STATUS)

    if target == 'fetch':
        return do_fetch()
    elif target == 'config':
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
