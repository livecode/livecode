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

BUILD_EDITION ?= community

# Those directories are given to the tool builder, and they might get passed
# (like private-dir) to engine functions, to which a path relative to this file
# becomes invalid).
top_src_dir=${PWD}
engine_dir=${top_src_dir}
output_dir=${top_src_dir}
work_dir=${top_src_dir}/_cache/builder_tool
private_dir=${top_src_dir}/..
bin_dir = ${top_src_dir}/$(BUILD_PLATFORM)-bin
docs_source_dir = ${top_src_dir}/docs
docs_private_source_dir = ${private_dir}/docs
docs_build_dir = ${top_src_dir}/_build/docs-build

ifeq ($(BUILD_PLATFORM),mac)
  LIVECODE = $(bin_dir)/LiveCode-Community.app/Contents/MacOS/LiveCode-Community
  buildtool_platform = mac
  UPLOAD_ENABLE_CHECKSUM ?= no
  UPLOAD_RELEASE_NOTES ?= no
else ifeq ($(BUILD_PLATFORM),linux-x86)
  LIVECODE = $(bin_dir)/LiveCode-Community
  buildtool_platform = linux
  UPLOAD_ENABLE_CHECKSUM ?= yes
  UPLOAD_RELEASE_NOTES ?= no
else ifeq ($(BUILD_PLATFORM),linux-x86_64)
  LIVECODE = $(bin_dir)/LiveCode-Community
  buildtool_platform = linux
  UPLOAD_ENABLE_CHECKSUM ?= yes
  UPLOAD_RELEASE_NOTES ?= yes
endif

# FIXME add --warn-as-error
buildtool_command = $(LIVECODE) -ui $(BUILDTOOL_STACK) \
	--build $(BUILD_STABILITY) \
	--engine-dir ${engine_dir} --output-dir ${output_dir} --work-dir ${work_dir} \
	--private-dir ${private_dir}

# Settings for upload
RSYNC ?= rsync
SHA1SUM ?= sha1sum
UPLOAD_SERVER ?= meg.on-rev.com
UPLOAD_PATH = staging/$(BUILD_LONG_VERSION)/$(GIT_VERSION)
UPLOAD_MAX_RETRIES = 50

dist-docs: dist-docs-community

ifeq ($(BUILD_EDITION),commercial)
dist-docs: dist-docs-commercial
endif

dist-docs-community:
	mkdir -p $(docs_build_dir)
	cp -R $(docs_source_dir) $(docs_build_dir)/raw-community
	$(buildtool_command) --platform $(buildtool_platform) --stage docs \
	  --docs-dir $(docs_build_dir)/raw-community \
	  --built-docs-dir $(docs_build_dir)/cooked-community
	  
dist-docs-commercial:
	mkdir -p $(docs_build_dir)
	cp -R $(docs_source_dir) $(docs_build_dir)/raw-commercial
	rsync -a $(docs_private_source_dir)/ $(docs_build_dir)/raw-commercial/
	$(buildtool_command) --platform $(buildtool_platform) --stage docs \
	  --docs-dir $(docs_build_dir)/raw-commercial \
	  --built-docs-dir $(docs_build_dir)/cooked-commercial

dist-notes:
	WKHTMLTOPDF=$(WKHTMLTOPDF) \
	$(buildtool_command) --platform $(buildtool_platform) --stage notes

ifeq ($(BUILD_EDITION),commercial)
dist-server: dist-server-commercial
endif

dist-server: dist-server-community

dist-server-community:
	$(buildtool_command) --platform mac --platform win --platform linux \
	    --stage server --edition community --warn-as-error

dist-server-commercial:
	$(buildtool_command) --platform mac --platform win --platform linux \
	    --stage server --edition commercial --warn-as-error

ifeq ($(BUILD_EDITION),commercial)
dist-tools: dist-tools-commercial
distmac-disk: distmac-disk-indy distmac-disk-business
endif

dist-tools: dist-tools-community
distmac-disk: distmac-disk-community

dist-tools-community:
	$(buildtool_command) --platform mac --platform win --platform linux --stage tools --edition community \
	  --built-docs-dir $(docs_build_dir)/cooked-community
dist-tools-commercial:
	$(buildtool_command) --platform mac --platform win --platform linux --stage tools --edition indy \
	  --built-docs-dir $(docs_build_dir)/cooked-commercial
	$(buildtool_command) --platform mac --platform win --platform linux --stage tools --edition business \
	  --built-docs-dir $(docs_build_dir)/cooked-commercial

# Make a list of installers to be uploaded to the distribution server, and release notes
# If a checksum file is needed, generate it with sha1sum
# Upload the release notes if we are on Linux
dist-upload-files.txt sha1sum.txt:
	set -e; \
	find . -maxdepth 1 -name 'LiveCode*Installer-*-Mac.dmg' \
	                -o -name 'LiveCode*Installer-*-Windows.exe' \
	                -o -name 'LiveCode*Installer-*-Linux.*' \
	                -o -name 'LiveCode*Server-*-Linux*.zip' \
	                -o -name 'LiveCode*Server-*-Mac.zip' \
	                -o -name 'LiveCode*Server-*-Windows.zip' \
	                -o -name '*-bin.tar.xz' \
	  > dist-upload-files.txt; \
	if test "${UPLOAD_RELEASE_NOTES}" = "yes"; then \
		find . -maxdepth 1 -name 'LiveCodeNotes*.pdf' >> dist-upload-files.txt; \
	fi; \
	if test "$(UPLOAD_ENABLE_CHECKSUM)" = "yes"; then \
	  $(SHA1SUM) < dist-upload-files.txt > sha1sum.txt; \
	  echo sha1sum.txt >> dist-upload-files.txt; \
	else \
	  touch sha1sum.txt; \
	fi

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

distmac-upload: dist-upload
