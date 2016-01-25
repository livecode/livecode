#!/usr/bin/env python

# Copyright (c) 2009 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys

# [[ Gyp customisation ]] Make sure we don't pull in a system Gyp
import os.path
sys.path.insert(0, os.path.join(os.path.dirname(sys.argv[0]), 'pylib'))
import gyp

if __name__ == '__main__':
  sys.exit(gyp.script_main())
