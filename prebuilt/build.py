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
  build.py [--target TARGET]

Options:
  -p, --target TARGET
                    Choose which target triple to build for
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
        return 'mac'
    if system == 'Linux':
        if re.match('^(x|i.?)86$', arch) is not None:
            return 'linux-x86'
        else:
            return 'linux-' + arch
    if system == 'Windows':
        if arch == 'AMD64':
            return 'win32-x86_64'
        else:
            return 'win32-x86'

################################################################
# Parse command-line options
################################################################

def process_env_options(opts):
    vars = ('TARGET',)
    for v in vars:
        opts[v] = os.getenv(v)

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
        if key in ('-p', '--target'):
            opts['TARGET'] = value
            offset += 2
            continue

        # Unrecognised option
        error("Unrecognised option '{}'".format(key))

################################################################
# Validate
################################################################

def validate_target(opts):
    target = opts['TARGET']
    if target is None:
        target = guess_target()
    if target is None:
        error("Cannot guess target; specify '--target <name>-'")

    opts['TARGET'] = target

################################################################
# Action
################################################################

def exec_build_libraries(build_platform, build_arch):
	# set curdir to prebuilt folder
	os.chdir(os.path.dirname(__file__))
	if platform.system() == 'Windows':
		args = ["build-all-libs.bat", build_platform, build_arch]
	else:
		args = ["./build-libraries.sh", build_platform, build_arch]
	print(' '.join(args))
	status = subprocess.call(args)
	if status != 0:
		sys.exit(status)

def build(args):
    opts = {}
    process_env_options(opts)
    process_arg_options(opts, args)

    validate_target(opts)
    
    # Get the target platform to pass to build-libraries
    target_platform, target_arch = opts['TARGET'].split('-')[0:2]

    print('Building target platform prebuilts (' + target_platform + ',' + target_arch + ')')
    exec_build_libraries(target_platform, target_arch)

if __name__ == '__main__':
    build(sys.argv[1:])
