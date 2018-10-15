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

################################################################
# Action
################################################################

def exec_upload_prebuilts():
	# set curdir to prebuilt folder
	os.chdir(os.path.dirname(__file__))
	if platform.system() == 'Windows':
		args = ["..\util\invoke-unix.bat", "./upload-libs.sh"]
	else:
		args = ["./upload-libs.sh"]
	print(' '.join(args))
	status = subprocess.call(args)
	if status != 0:
		sys.exit(status)

def upload(args):
    print('Uploading all platform prebuilts')
    exec_upload_prebuilts()

if __name__ == '__main__':
    upload(sys.argv[1:])
