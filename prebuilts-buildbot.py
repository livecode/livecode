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
#import fetch

# The set of build tasks that this branch supports
BUILDBOT_TARGETS = ('fetch', 'config', 'compile', 'bin-archive', 'bin-extract', 'prebuilts-upload')

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

def split_target_triple():
	# Fetch target triple as 3-tuple of (platform, arch, subplatform)
	check_target_triple()
	target_components = get_target_triple().split('-')
	# add empty values for missing components
	while len(target_components) < 3:
		target_components.append(None)

	target_arch = target_components[0]
	target_platform = target_components[1]
	target_subplatform = target_components[2]

	return (target_platform, target_arch, target_subplatform)

################################################################
# Fetch prebuilts
################################################################

def do_fetch():
	print('skip fetch step')
	#check_target_triple()
	#exec_fetch(['--target', get_target_triple()])

################################################################
# Configure with gyp
################################################################

def format_target_params(platform, arch, subplatform):
	args = []
	if platform is not None:
		args.extend(['--target-platform', platform])
	if arch is not None:
		args.extend(['--target-arch', arch])
	if subplatform is not None:
		args.extend(['--target-subplatform', subplatform])
	return args

def build(target):
	args = ["python", "prebuilt/build.py"] + format_target_params(*target)
	print(' '.join(args))

	exit_status = subprocess.call(args)
	if exit_status == 0 and get_build_edition() == "commercial":
		args = ["python", "../prebuilt/build.py"] + format_target_params(*target)
		print(' '.join(args))
		exit_status = subprocess.call(args)

	return exit_status
	
def package(target):
	args = ["python", "prebuilt/package.py"] + format_target_params(*target)
	print(' '.join(args))

	exit_status = subprocess.call(args)
	if exit_status == 0 and get_build_edition() == "commercial":
		args = ["python", "../prebuilt/package.py"] + format_target_params(*target)
		print(' '.join(args))
		exit_status = subprocess.call(args)

	return exit_status
	
def do_configure():
	target = split_target_triple()
	# perform build + package + upload in configure step

	exit_status = build(target)
	if exit_status == 0:
		exit_status = package(target)

	return exit_status

################################################################
# Compile
################################################################

def do_compile():
	print('skip compile')

################################################################
# Archive / extract built binaries
################################################################

def bin_archive(target):
	args = ["python", "prebuilt/archive.py"] + format_target_params(*target)
	print(' '.join(args))

	exit_status = subprocess.call(args)
	if exit_status == 0 and get_build_edition() == "commercial":
		args = ["python", "../prebuilt/archive.py"] + format_target_params(*target)
		print(' '.join(args))
		exit_status = subprocess.call(args)

	return exit_status

def do_bin_archive():
	target = split_target_triple()
	return bin_archive(target)

def do_bin_extract():
	exit_status = subprocess.call(['python', 'prebuilt/extract.py'])
	if exit_status == 0 and get_build_edition() == "commercial":
		args = ["python", "../prebuilt/extract.py"]
		exit_status = subprocess.call(args)

	return exit_status

################################################################
# Upload packaged binaries
################################################################

def prebuilts_upload(target):
	args = ["python", "prebuilt/upload.py"] + format_target_params(*target)
	print(' '.join(args))

	exit_status = subprocess.call(args)
	if exit_status == 0 and get_build_edition() == "commercial":
		args = ["python", "../prebuilt/upload.py"] + format_target_params(*target)
		print(' '.join(args))
		exit_status = subprocess.call(args)

	return exit_status

def do_prebuilts_upload():
	target = split_target_triple()
	# perform build + package + upload in configure step

	return prebuilts_upload(target)

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
    elif target == 'bin-extract':
        return do_bin_extract()
    elif target == 'prebuilts-upload':
        return do_prebuilts_upload()

if __name__ == '__main__':
    if len(sys.argv) < 2:
        error("You must specify a buildbot target stage")
    sys.exit(buildbot_task(sys.argv[1]))
