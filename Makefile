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

.DEFAULT: all

XCODEBUILD ?= xcodebuild

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

compile-linux-%: | config-linux-%
	$(MAKE) -C build-linux-$*

.PHONY: all-linux
.PHONY: config-linux $(addprefix config-linux-,$(LINUX_ARCHS))
.PHONY: compile-linux $(addprefix compile-linux-,$(LINUX_ARCHS))

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

.PHONY: all-android config-android compile-android

################################################################
# Mac rules
################################################################

config-mac:
	./config.sh --platform mac-all

compile-mac:
	$(XCODEBUILD) -project build-mac-all/livecode.xcodeproj

all-mac:
	$(MAKE) config-mac
	$(MAKE) compile-mac

.PHONY: all-mac config-mac compile-mac

################################################################
# iOS rules
################################################################

IOS_SDKS = \
	iphoneos8.3 \
	iphoneos8.2 \
	iphonesimulator8.3 \
	iphonesimulator8.2 \
	iphonesimulator7.1 \
	iphonesimulator6.1 \
	iphonesimulator5.1

all-ios-%:
	$(MAKE) config-ios-$*
	$(MAKE) compile-ios-$*

config-ios-%:
	./config.sh --platform ios-all --generator-output build-ios-all-$* -Dtarget_sdk=$*

compile-ios-%:
	$(XCODEBUILD) -project build-ios-all-$*/livecode.xcodeproj

# Provide some synonyms for "latest iOS SDK"
$(addsuffix -ios-iphoneos,all config compile): %: %8.3
	@true
$(addsuffix -ios-iphonesimulator,all config compile): %: %8.3
	@true

all-ios: $(addprefix all-ios-,$(IOS_SDKS))
config-ios: $(addprefix config-ios-,$(IOS_SDKS))
compile-ios: $(addprefix compile-ios-,$(IOS_SDKS))
