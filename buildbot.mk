# Copyright (C) 2015 Runtime Revolution Ltd.
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

# This file contains rules used by the LiveCode Buildbot installation at
# <https://vulcan.livecode.com/>

################################################################
# Configure with gyp
################################################################

# Buildbot must set the variables PLATFORM and SUBPLATFORM

ifeq ($(BUILD_SUBPLATFORM),)
CONFIG_TARGET = config-$(BUILD_PLATFORM)
else
CONFIG_TARGET = config-$(BUILD_PLATFORM)-$(BUILD_SUBPLATFORM)
endif

config:
	$(MAKE) $(CONFIG_TARGET)

.PHONY: config

################################################################
# Compile
################################################################

# Buildbot must set the variables PLATFORM and SUBPLATFORM

ifeq ($(BUILD_SUBPLATFORM),)
COMPILE_TARGET = compile-$(BUILD_PLATFORM)
else
COMPILE_TARGET = compile-$(BUILD_PLATFORM)-$(BUILD_SUBPLATFORM)
endif

compile:
	$(MAKE) $(COMPILE_TARGET)

.PHONY: compile

################################################################
# Archive / extract built binaries
################################################################

bin-archive:
	tar -Jcvf $(BUILD_PLATFORM)-bin.tar.xz $(BUILD_PLATFORM)-bin

bin-extract:
	find . -maxdepth 1 -name '*-bin.xz' -print0 | xargs tar -x -n1 -f

################################################################
# Installer generation
################################################################

# TBD
