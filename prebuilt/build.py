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

import sys
import platform
import os
import subprocess
import re

def usage(exit_status):
    print(
"""Compile prebuilts needed to build LiveCode.

Usage:
  build.py [--target-platform PLATFORM] [--target-arch ARCH] [--target-subplatform SUBPLATFORM]

Options:
  -p, --target-platform TARGET
                    Choose which target platform to build for
  -a, --target-arch ARCH
                    Choose which target arch to build for
  -s, --target-subplatform SUBPLATFORM
                    Choose which target subplatform to build for
  -h, --help        Print this message
""")
    for p in KNOWN_PLATFORMS:
        print("  " + p)
    sys.exit(exit_status)

def error(message):
    print('ERROR: ' + message)
    sys.exit(1)

def guess_target():
    system = platform.system()
    arch = platform.machine()
    if system == 'Darwin':
        return ('mac', 'x86_64')
    if system == 'Linux':
        if re.match('^(x|i.?)86$', arch) is not None:
            return ('linux', 'x86')
        else:
            return ('linux', arch)
    if system == 'Windows':
        if arch == 'AMD64':
            return ('win32', 'x86_64')
        else:
            return ('win32', 'x86')

	# could not identify host platform + arch
	return (None, None)

################################################################
# Parse command-line options
################################################################

def process_default_options(opts):
	target = guess_target()
	opts['TARGET_PLATFORM'] = target[0]
	opts['TARGET_ARCH'] = target[1]
	opts['TARGET_SUBPLATFORM'] = None

def process_env_options(opts):
    vars = ('TARGET_PLATFORM','TARGET_ARCH','TARGET_SUBPLATFORM')
    for v in vars:
		value = os.getenv(v)
		if value is not None:
			opts[v] = value

def process_arg_options(opts, args):
    offset = 0
    while offset < len(args):
        key = args[offset]
        if offset + 1 < len(args):
            value = args[offset + 1]
        else:
            value = None

        if key in ('-h', '--help'):
            usage(0)
        if key in ('-p', '--target-platform'):
            opts['TARGET_PLATFORM'] = value
            offset += 2
            continue
        if key in ('-a', '--target-arch'):
            opts['TARGET_ARCH'] = value
            offset += 2
            continue
        if key in ('-s', '--target-subplatform'):
            opts['TARGET_SUBPLATFORM'] = value
            offset += 2
            continue

        # Unrecognised option
        error("Unrecognised option '{}'".format(key))

################################################################
# Validate
################################################################

def validate_target(opts):
	if opts['TARGET_PLATFORM'] is None:
		error("Cannot guess target platform; specify '--target-platform <name>'")

	if opts['TARGET_ARCH'] is None:
		error("Cannot guess target arch; specify '--target-arch <name>'")

	# Subplatform may be unspecified
	if opts['TARGET_SUBPLATFORM'] is None:
		opts['TARGET_SUBPLATFORM'] = ''
		
################################################################
# Action
################################################################

def exec_build_libraries(build_platform, build_arch, build_subplatform):
	# set curdir to prebuilt folder
	os.chdir(os.path.dirname(__file__))
	if platform.system() == 'Windows':
		args = ["build-all-libs.bat", build_platform, build_arch, build_subplatform]
	else:
		args = ["./build-libraries.sh", build_platform, build_arch, build_subplatform]
	print(' '.join(args))
	status = subprocess.call(args)
	if status != 0:
		sys.exit(status)

def build(args):
    opts = {}
    process_default_options(opts)
    process_env_options(opts)
    process_arg_options(opts, args)

    validate_target(opts)

    print('Building target platform prebuilts (' + opts['TARGET_PLATFORM'] + ',' + opts['TARGET_ARCH'] + ',' + opts['TARGET_SUBPLATFORM'] + ')')
    exec_build_libraries(opts['TARGET_PLATFORM'], opts['TARGET_ARCH'], opts['TARGET_SUBPLATFORM'])

if __name__ == '__main__':
    build(sys.argv[1:])
