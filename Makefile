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
IPHONEOS_VERSIONS ?= 10.2 11.2 12.1 13.2 13.5
IPHONESIMULATOR_VERSIONS ?= 10.2 11.2 12.1 13.2 13.5
SKIP_IPHONEOS_VERSIONS ?= 9.2
SKIP_IPHONESIMULATOR_VERSIONS ?= 6.1 7.1 8.2 9.2


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
XCODEBUILD_FILTER ?=

include Makefile.common

################################################################

.DEFAULT: all

all: all-$(guess_platform)
check: check-$(guess_platform)

# [[ MDW-2017-05-09 ]] feature_clean_target
clean-linux:
	rm -rf linux-*-bin
	rm -rf build-linux-*
	rm -rf prebuilt/fetched
	rm -rf prebuilt/include
	rm -rf prebuilt/lib
	find . -name \*.lcb | xargs touch

check-common-%:
ifneq ($(TRAVIS),undefined)
	@echo "travis_fold:start:testengine"
	@echo "TEST Engine"
endif
	$(MAKE) -C tests bin_dir=../$*-bin
ifneq ($(TRAVIS),undefined)
	@echo "travis_fold:end:testengine"
	@echo "travis_fold:start:testide"
	@echo "TEST IDE"
endif
	$(MAKE) -C ide/tests bin_dir=../../$*-bin
ifneq ($(TRAVIS),undefined)
	@echo "travis_fold:end:testide"
	@echo "travis_fold:start:testextensions"
	@echo "TEST Extensions"
endif
	$(MAKE) -C extensions bin_dir=../$*-bin
ifneq ($(TRAVIS),undefined)
	@echo "travis_fold:end:testextensions"
endif
################################################################
# Linux rules
################################################################

LINUX_ARCHS = x86_64 x86 armv6hf armv7

config-linux-%:
ifneq ($(TRAVIS),undefined)
	@echo "travis_fold:start:config"
	@echo "CONFIGURE"
endif
	./config.sh --platform linux-$*
ifneq ($(TRAVIS),undefined)
	@echo "travis_fold:end:config"
endif
	
compile-linux-%:
ifneq ($(TRAVIS),undefined)
	@echo "travis_fold:start:compile"
	@echo "COMPILE"
endif
	$(MAKE) -C build-linux-$*/livecode default
ifneq ($(TRAVIS),undefined)
	@echo "travis_fold:end:compile"
endif
	
check-linux-%:
ifneq ($(TRAVIS),undefined)
	@echo "travis_fold:start:testcpp"
	@echo "TEST C++"
endif
	$(MAKE) -C build-linux-$*/livecode check
ifneq ($(TRAVIS),undefined)
	@echo "travis_fold:end:testcpp"
endif
	$(MAKE) check-common-linux-$*

all-linux-%:
	$(MAKE) config-linux-$*
	$(MAKE) compile-linux-$*

$(addsuffix -linux,all config compile check): %: %-$(guess_linux_arch)

################################################################
# Android rules
################################################################

ANDROID_ARCHS = armv6 armv7 arm64 x86 x86_64

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
ifneq ($(TRAVIS),undefined)
	@echo "travis_fold:start:config"
	@echo "CONFIGURE"
endif
	./config.sh --platform mac
ifneq ($(TRAVIS),undefined)
	@echo "travis_fold:end:config"
endif
	
compile-mac:
ifneq ($(TRAVIS),undefined)
	@echo "travis_fold:start:compile"
	@echo "COMPILE"
endif
	$(XCODEBUILD) -project "build-mac$(BUILD_SUBDIR)/$(BUILD_PROJECT).xcodeproj" -configuration $(BUILDTYPE) -target default \
	  $(XCODEBUILD_FILTER)
ifneq ($(TRAVIS),undefined)
	@echo "travis_fold:end:compile"
endif
	  
check-mac:
ifneq ($(TRAVIS),undefined)
	@echo "travis_fold:start:testcpp"
	@echo "TEST C++"
endif
	$(XCODEBUILD) -project "build-mac$(BUILD_SUBDIR)/$(BUILD_PROJECT).xcodeproj" -configuration $(BUILDTYPE) -target check \
	  $(XCODEBUILD_FILTER)
ifneq ($(TRAVIS),undefined)
	@echo "travis_fold:end:testcpp"
endif
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

# Dummy targets to prevent our build system from building old iOS simulators+devices
$(addprefix config-ios-iphonesimulator,$(SKIP_IPHONESIMULATOR_VERSIONS)):
	@echo "Skipping $@ (no longer supported)"
$(addprefix compile-ios-iphonesimulator,$(SKIP_IPHONESIMULATOR_VERSIONS)):
	@echo "Skipping $@ (no longer supported)"
$(addprefix check-ios-iphonesimulator,$(SKIP_IPHONESIMULATOR_VERSIONS)):
	@echo "Skipping $@ (no longer supported)"
	
$(addprefix config-ios-iphonesimulator,$(SKIP_IPHONEOS_VERSIONS)):
	@echo "Skipping $@ (no longer supported)"
$(addprefix compile-ios-iphonesimulator,$(SKIP_IPHONEOS_VERSIONS)):
	@echo "Skipping $@ (no longer supported)"
$(addprefix check-ios-iphonesimulator,$(SKIP_IPHONEOS_VERSIONS)):
	@echo "Skipping $@ (no longer supported)"

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

# Dummy rules for Windows x86-64 builds
# TODO Replace with real rules
config-win-x86_64:
	mkdir -p build-win-x86_64
compile-win-x86_64:
	mkdir -p win-x86_64-bin
all-win-x86_64:
	$(MAKE) config-win-x86_64
	$(MAKE) compile-win-x86_64

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
