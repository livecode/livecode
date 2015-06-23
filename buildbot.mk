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

# Load version information
include version

# Get git commit information
ifeq ($(BUILD_EDITION),commercial)
GIT_VERSION=g$(shell git --git-dir=../.git rev-parse --short HEAD)
else
GIT_VERSION=g$(shell git rev-parse --short HEAD)
endif

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
	find . -maxdepth 1 -name '*-bin.tar.xz' -print0 | xargs -0 -n1 tar -xvf

################################################################
# Installer generation
################################################################

# BUILD_PLATFORM will be set to the platform on which the installer's being
# built.  Its build artefacts will have been extracted into the
# ./$(BUILD_PLATFORM)-bin/ directory

BUILD_STABILITY ?= beta

BUILDTOOL_STACK = builder/builder_tool.livecodescript

WKHTMLTOPDF ?= $(shell which wkhtmltopdf 2>/dev/null)

bin_dir = $(BUILD_PLATFORM)-bin

ifeq ($(BUILD_PLATFORM),mac)
  LIVECODE = $(bin_dir)/LiveCode-Community.app/Contents/MacOS/LiveCode-Community
  buildtool_platform = mac
else ifeq ($(BUILD_PLATFORM),linux-x86)
  LIVECODE = $(bin_dir)/livecode-community
  buildtool_platform = linux
else ifeq ($(BUILD_PLATFORM),linux-x86_64)
  LIVECODE = $(bin_dir)/livecode-community
  buildtool_platform = linux
endif

# FIXME add --warn-as-error
buildtool_command = $(LIVECODE) -ui $(BUILDTOOL_STACK) \
	--build $(BUILD_STABILITY) \
	--engine-dir . --output-dir . --work-dir ./_cache/builder_tool \
	--private-dir ..

# Settings for upload
RSYNC ?= rsync
UPLOAD_SERVER ?= meg.on-rev.com
UPLOAD_PATH = staging/$(BUILD_LONG_VERSION)/$(GIT_VERSION)
UPLOAD_MAX_RETRIES = 50

dist-docs:
	$(buildtool_command) --platform $(buildtool_platform) --stage docs

dist-notes:
	WKHTMLTOPDF=$(WKHTMLTOPDF) \
	$(buildtool_command) --platform $(buildtool_platform) --stage notes

ifeq ($(BUILD_EDITION),commercial)
dist-server: dist-server-commercial
endif

dist-server: dist-server-community

dist-server-community:
	$(buildtool_command) --platform mac --platform win --platform linux \
	    --stage server --edition community

dist-server-commercial:
	$(buildtool_command) --platform mac --platform win --platform linux \
	    --stage server --edition commercial

ifeq ($(BUILD_EDITION),commercial)
dist-tools: dist-tools-commercial
distmac-disk: distmac-disk-commercial
endif

dist-tools: dist-tools-community
distmac-disk: distmac-disk-community

dist-tools-community:
	$(buildtool_command) --platform linux --platform mac --platform win --stage tools --edition community
dist-tools-commercial:
	$(buildtool_command) --platform linux --platform mac --platform win --stage tools --edition commercial

# Make a list of installers to be uploaded to the distribution server
dist-upload-files.txt:
	find . -maxdepth 1 -name 'LiveCode*Installer-*-Mac.dmg' \
	                -o -name 'LiveCode*Installer-*-Windows.exe' \
	                -o -name 'LiveCode*Installer-*-Linux.*' \
	  > $@

# Perform the upload.  This is in two steps:
# (1) Create the target directory
# (2) Transfer the files using rsync
#
# We need to do the actual transfer in a loop to deal with possible
# connection drops
dist-upload-mkdir:
	ssh $(UPLOAD_SERVER) "mkdir -p \"$(UPLOAD_PATH)\""
dist-upload: dist-upload-files.txt dist-upload-mkdir
	trap "echo Interrupted; exit;" SIGINT SIGTERM; \
	i=0; \
	false; \
	while [ $$? -ne 0 -a $$i -lt $(UPLOAD_MAX_RETRIES) ] ; do \
	  i=$$(($$i+1)); \
	  rsync -v --progress --partial --chmod=ugo=rwX --executability \
	    --files-from=dist-upload-files.txt . $(UPLOAD_SERVER):"\"$(UPLOAD_PATH)\""; \
	done; \
	rc=$$?; \
	if [ $$i -eq $(UPLOAD_MAX_RETRIES) ]; then \
	  echo "Maximum retries reached, giving up"; \
	fi; \
	exit $$rc

# This rule is used for packing the Mac installer contents; the
# resulting archive gets transferred to a Mac for signing and
# conversion to a DMG.
distmac-archive:
	find . -maxdepth 1 -name 'LiveCode*Installer-*-Mac.app' -print0 \
	    | xargs -0 tar -Jcvf mac-installer.tar.xz

distmac-extract:
	tar -xvf mac-installer.tar.xz

# Final installer creation for Mac
distmac-disk-%:
	$(buildtool_command) --platform mac --stage disk --edition $*
