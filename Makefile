# Copyright (C) 2015 LiveCode Ltd.
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
EMMAKE ?= emmake

# Some magic to control which versions of iOS we try to build.  N.b. you may
# also need to modify the buildbot configuration
IPHONEOS_VERSIONS ?= 8.2 9.2 10.0
IPHONESIMULATOR_VERSIONS ?= 6.1 7.1 8.2 9.2 10.0

IOS_SDKS ?= \
	$(addprefix iphoneos,$(IPHONEOS_VERSIONS)) \
	$(addprefix iphonesimulator,$(IPHONESIMULATOR_VERSIONS))

# Choose the correct build type
MODE ?= debug

# Where to run the build command depends on community vs commercial
ifeq ($(BUILD_EDITION),commercial)
  BUILD_SUBDIR :=
  BUILD_PROJECT := livecode-commercial
else
  BUILD_SUBDIR := /livecode
  BUILD_PROJECT := livecode
endif

# Prettifying output for CI builds
ifeq ($(TRAVIS),true)
  XCODEBUILD := set -o pipefail && $(XCODEBUILD)
  XCODEBUILD_FILTER := | xcpretty
else
  XCODEBUILD_FILTER :=
endif 

include Makefile.common

################################################################

.DEFAULT: all

all: all-$(guess_platform)
check: check-$(guess_platform)

check-common-%:
	$(MAKE) -C tests bin_dir=../$*-bin
	$(MAKE) -C ide/tests bin_dir=../../$*-bin
	$(MAKE) -C extensions bin_dir=../$*-bin

################################################################
# Linux rules
################################################################

LINUX_ARCHS = x86_64 x86

config-linux-%:
	./config.sh --platform linux-$*

compile-linux-%:
	$(MAKE) -C build-linux-$*/livecode default

check-linux-%:
	$(MAKE) -C build-linux-$*/livecode check
	$(MAKE) check-common-linux-$*

all-linux-%:
	$(MAKE) config-linux-$*
	$(MAKE) compile-linux-$*

$(addsuffix -linux,all config compile check): %: %-$(guess_linux_arch)

################################################################
# Android rules
################################################################

ANDROID_ARCHS = armv6

config-android-%:
	./config.sh --platform android-$*

compile-android-%:
	$(MAKE) -C build-android-$*/livecode default

check-android-%:
	$(MAKE) -C build-android-$*/livecode check

all-android-%:
	$(MAKE) config-android-$*
	$(MAKE) compile-android-$*

$(addsuffix -android,all config compile check): %: %-armv6

################################################################
# Mac rules
################################################################

config-mac:
	./config.sh --platform mac

compile-mac:
	$(XCODEBUILD) -project "build-mac$(BUILD_SUBDIR)/$(BUILD_PROJECT).xcodeproj" -configuration $(BUILDTYPE) -target default \
	  $(XCODEBUILD_FILTER)

check-mac:
	$(XCODEBUILD) -project "build-mac$(BUILD_SUBDIR)/$(BUILD_PROJECT).xcodeproj" -configuration $(BUILDTYPE) -target check \
	  $(XCODEBUILD_FILTER)
	$(MAKE) check-common-mac


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
	./config.sh --platform ios --generator-output build-ios-$*/livecode -Dtarget_sdk=$*

compile-ios-%:
	$(XCODEBUILD) -project "build-ios-$*$(BUILD_SUBDIR)/$(BUILD_PROJECT).xcodeproj" -configuration $(BUILDTYPE) -target default

check-ios-%:
	$(XCODEBUILD) -project "build-ios-$*$(BUILD_SUBDIR)/$(BUILD_PROJECT).xcodeproj" -configuration $(BUILDTYPE) -target check

# Dummy targets to prevent our build system from building iOS 5.1 simulator
config-ios-iphonesimulator5.1:
	@echo "Skipping iOS simulator 5.1 (no longer supported)"
compile-ios-iphonesimulator5.1:
	@echo "Skipping iOS simulator 5.1 (no longer supported)"
check-ios-iphonesimulator5.1:
	@echo "Skipping iOS simulator 5.1 (no longer supported)"

# Provide some synonyms for "latest iOS SDK"
$(addsuffix -ios-iphoneos,all config compile check): %: %$(lastword $(IPHONEOS_VERSIONS))
	@true
$(addsuffix -ios-iphonesimulator,all config compile check): %: %$(lastword ($IPHONESIMULATOR_VERSIONS))
	@true

all_ios_subplatforms = iphoneos iphonesimulator $(IOS_SDKS)

all-ios: $(addprefix all-ios-,$(IOS_SDKS))
config-ios: $(addprefix config-ios-,$(IOS_SDKS))
compile-ios: $(addprefix compile-ios-,$(IOS_SDKS))
check-ios: $(addprefix check-ios-,$(IOS_SDKS))

################################################################
# Windows rules
################################################################

config-win-%:
	./config.sh --platform win-$*

compile-win-%:
	# windows builds occur under Wine
	cd build-win-$* && $(WINE) /K ../make.cmd

check-win-%:
	# windows builds occur under Wine
	cd build-win-$* && $(WINE) /K ../make.cmd check
	$(MAKE) check-common-win-$*

all-win-%:
	$(MAKE) config-win-$*
	$(MAKE) compile-win-$*

$(addsuffix -win,all config compile): %: %-x86

################################################################
# Emscripten rules
################################################################

config-emscripten:
	$(EMMAKE) ./config.sh --platform emscripten

compile-emscripten:
	$(EMMAKE) $(MAKE) -C build-emscripten/livecode default

check-emscripten:
	$(EMMAKE) $(MAKE) -C build-emscripten/livecode check

all-emscripten:
	$(MAKE) config-emscripten
	$(MAKE) compile-emscripten
