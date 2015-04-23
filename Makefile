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

# Usually, you'll just want to type "make all".

################################################################

# Tools that Make calls
XCODEBUILD ?= xcodebuild
WINE ?= wine

# Some magic to control which versions of iOS we try to build.  N.b. you may
# also need to modify the buildbot configuration
IPHONEOS_VERSIONS ?= 8.2 8.3
IPHONESIMULATOR_VERSIONS ?= 5.1 6.1 7.1 8.2 8.3

IOS_SDKS ?= \
	$(addprefix iphoneos,$(IPHONEOS_VERSIONS)) \
	$(addprefix iphonesimulator,$(IPHONESIMULATOR_VERSIONS))

# Choose the correct build type
MODE ?= debug
ifeq ($(MODE),debug)
  export BUILDTYPE ?= Debug
else ifeq ($(MODE),release)
  export BUILDTYPE ?= Release
else
  $(error "Mode must be 'debug' or 'release'")
endif

################################################################

.DEFAULT: all

guess_platform_script := \
	case `uname -s` in \
		Linux) echo linux ;; \
		Darwin) echo mac ;; \
	esac
guess_platform := $(shell $(guess_platform_script))

all: all-$(guess_platform)

################################################################
# Linux rules
################################################################

guess_linux_arch_script := \
	case `uname -p` in \
		x86_64) echo x86_64 ;; \
		x86|i*86) echo x86 ;; \
	esac

guess_linux_arch := $(shell $(guess_linux_arch_script))

all-linux:
	$(MAKE) config-linux-$(guess_linux_arch)
	$(MAKE) compile-linux-$(guess_linux_arch)

config-linux: config-linux-$(guess_linux_arch)
compile-linux: compile-linux-$(guess_linux_arch)

LINUX_ARCHS = x86_64 x86

config-linux-%:
	./config.sh --platform linux-$*

compile-linux-%:
	$(MAKE) -C build-linux-$*

################################################################
# Android rules
################################################################

config-android:
	./config.sh --platform android-armv6

compile-android:
	$(MAKE) -C build-android-armv6

all-android:
	$(MAKE) config-android
	$(MAKE) compile-android

################################################################
# Mac rules
################################################################

config-mac:
	./config.sh --platform mac

compile-mac:
	$(XCODEBUILD) -project build-mac/livecode.xcodeproj -configuration $(BUILDTYPE)

all-mac:
	$(MAKE) config-mac
	$(MAKE) compile-mac

################################################################
# iOS rules
################################################################

all-ios-%:
	$(MAKE) config-ios-$*
	$(MAKE) compile-ios-$*

config-ios-%:
	./config.sh --platform ios --generator-output build-ios-$* -Dtarget_sdk=$*

compile-ios-%:
	$(XCODEBUILD) -project build-ios-$*/livecode.xcodeproj -configuration $(BUILDTYPE)

# Provide some synonyms for "latest iOS SDK"
$(addsuffix -ios-iphoneos,all config compile): %: %8.3
	@true
$(addsuffix -ios-iphonesimulator,all config compile): %: %8.3
	@true

all_ios_subplatforms = iphoneos iphonesimulator $(IOS_SDKS)

all-ios: $(addprefix all-ios-,$(IOS_SDKS))
config-ios: $(addprefix config-ios-,$(IOS_SDKS))
compile-ios: $(addprefix compile-ios-,$(IOS_SDKS))

################################################################
# Windows rules
################################################################

config-win:
	./config.sh --platform win-x86

compile-win:
	# windows builds occur under Wine
	cd build-win-x86 && $(WINE) /K ../make.cmd

all-win:
	$(MAKE) config-win
	$(MAKE) compile-win
