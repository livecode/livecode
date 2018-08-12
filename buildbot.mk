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
#
# Tasks that may be run on Windows workers must be implemented in the
# buildbot.py script.

# Load version information
include version

GIT_HASH_HEXIT_COUNT=10

# Get git commit information
ifeq ($(BUILD_EDITION),commercial)
GIT_VERSION=g$(shell git --git-dir=../.git rev-parse --short=$(GIT_HASH_HEXIT_COUNT) HEAD)
else
GIT_VERSION=g$(shell git rev-parse --short=$(GIT_HASH_HEXIT_COUNT) HEAD)
endif

################################################################
# Extract built binaries
################################################################

bin-extract:
	find . -maxdepth 1 -name '*-bin.tar.*' -exec tar -xvf '{}' ';'

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
ifeq ($(BUILD_EDITION),commercial)
  UPLOAD_RELEASE_NOTES ?= yes
else
  UPLOAD_RELEASE_NOTES ?= no
endif
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

dist-docs: dist-docs-api dist-docs-guide

dist-docs-api:
	mkdir -p $(docs_build_dir)
	$(buildtool_command) --platform $(buildtool_platform) --stage docs \
	  --built-docs-dir $(docs_build_dir)
	  
dist-notes:
	WKHTMLTOPDF=$(WKHTMLTOPDF) \
	$(buildtool_command) --platform $(buildtool_platform) \
	  --stage notes --warn-as-error \
	  --built-docs-dir $(docs_build_dir)

dist-docs-guide:
	WKHTMLTOPDF=$(WKHTMLTOPDF) \
	$(buildtool_command) --platform $(buildtool_platform) \
		--stage guide --warn-as-error

ifeq ($(BUILD_EDITION),commercial)
dist-server: dist-server-communityplus dist-server-indy dist-server-business
endif

dist-server: dist-server-community

dist-server-community:
	$(buildtool_command) --platform mac --platform win --platform linux \
	    --stage server --edition community --warn-as-error

dist-server-communityplus:
	$(buildtool_command) --platform mac --platform win --platform linux \
	    --stage server --edition communityplus --warn-as-error

dist-server-indy:
	$(buildtool_command) --platform mac --platform win --platform linux \
	    --stage server --edition indy --warn-as-error

dist-server-business:
	$(buildtool_command) --platform mac --platform win --platform linux \
		--stage server --edition business --warn-as-error
		
ifeq ($(BUILD_EDITION),commercial)
dist-tools: dist-tools-commercial
distmac-disk: distmac-disk-communityplus distmac-disk-indy distmac-disk-business
endif

dist-tools: dist-tools-community dist-tools-version-check
distmac-disk: distmac-disk-community

dist-tools-community:
	$(buildtool_command) --platform mac --platform win --platform linux --stage tools --edition community \
	  --built-docs-dir $(docs_build_dir)
dist-tools-commercial:
	$(buildtool_command) --platform mac --platform win --platform linux --stage tools --edition communityplus \
	  --built-docs-dir $(docs_build_dir)
	$(buildtool_command) --platform mac --platform win --platform linux --stage tools --edition indy \
	  --built-docs-dir $(docs_build_dir)
	$(buildtool_command) --platform mac --platform win --platform linux --stage tools --edition business \
  	  --built-docs-dir $(docs_build_dir)
# Ensure that the version for which we're trying to build installers
# hasn't already been tagged.
dist-tools-version-check:
	@git tag -l | xargs git tag -d ;\
	git fetch --tags ;\
	if git rev-parse refs/tags/$(BUILD_SHORT_VERSION) \
	        >/dev/null 2>&1 ; then \
	  echo; \
	  echo "$(BUILD_SHORT_VERSION) has already been released."; \
	  echo "You probably need to update the 'version' file."; \
	  echo; \
	  exit 1; \
	fi

.PHONY: dist-tools-version-check

distmac-bundle-community:
	$(buildtool_command) --platform mac --stage bundle --edition community
distmac-bundle-communityplus:
	$(buildtool_command) --platform mac --stage bundle --edition communityplus
distmac-bundle-indy:
	$(buildtool_command) --platform mac --stage bundle --edition indy
distmac-bundle-business:
	$(buildtool_command) --platform mac --stage bundle --edition business

# Make a list of installers to be uploaded to the distribution server, and release notes
# If a checksum file is needed, generate it with sha1sum
# Upload the release notes if we are on Linux
dist-upload-files.txt sha1sum.txt:
	set -e; \
	find . -maxdepth 1 -name 'LiveCode*-*-Mac.dmg' \
	                -o -name 'LiveCode*Installer-*-Windows.exe' \
	                -o -name 'LiveCode*Installer-*-Linux.*' \
	                -o -name 'LiveCode*Server-*-Linux*.zip' \
	                -o -name 'LiveCode*Server-*-Mac.zip' \
	                -o -name 'LiveCode*Server-*-Windows.zip' \
	                -o -name 'LiveCode*Docs-*.zip' \
	                -o -name '*-bin.tar.xz' \
	                -o -name '*-bin.tar.bz2' \
	                -o -name 'LiveCodeForFM-Mac-Solution.zip' \
	                -o -name 'LiveCodeForFM-Mac-Plugin.zip' \
	                -o -name 'LiveCodeForFM-Win-x86-Solution.zip' \
	                -o -name 'LiveCodeForFM-Win-x86-Plugin.zip' \
	                -o -name 'LiveCodeForFM-Win-x86_64-Solution.zip' \
	                -o -name 'LiveCodeForFM-Win-x86_64-Plugin.zip' \
	                -o -name 'LiveCodeForFM-All-Solutions.zip' \
	                -o -name 'LiveCodeForFM-All-Plugins.zip' \
	                -o -name 'LiveCodeForFM-Solution.zip' \
						 -o -name 'LiveCodeForFM.zip' \
 	  > dist-upload-files.txt; \
	if test "${UPLOAD_RELEASE_NOTES}" = "yes"; then \
		find . -maxdepth 1 -name 'LiveCodeNotes*.pdf' >> dist-upload-files.txt; \
		find . -maxdepth 1 -name 'LiveCodeNotes*.html' >> dist-upload-files.txt; \
		find . -maxdepth 1 -name 'LiveCodeUpdates*.md' >> dist-upload-files.txt; \
		find . -maxdepth 1 -name 'LiveCodeUpdates*.html' >> dist-upload-files.txt; \
		find . -maxdepth 1 -name 'LiveCodeUserGuide*.html' >> dist-upload-files.txt; \
		find . -maxdepth 1 -name 'LiveCodeUserGuide*.pdf' >> dist-upload-files.txt; \
	fi; \
	if test "$(UPLOAD_ENABLE_CHECKSUM)" = "yes"; then \
	  xargs --arg-file=dist-upload-files.txt $(SHA1SUM) > sha1sum.txt; \
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
	set -e; \
	find . -maxdepth 1 -name 'LiveCode*Installer-*-Mac.app' -print0 \
	    | xargs -0 tar -cvf mac-installer.tar; \
	cd mac-bin; \
	find . -maxdepth 1 -name 'livecodeforfm-*.fmplugin' -print0 \
	    | xargs -0 tar --append --file=../mac-installer.tar; \
	cd ..; \
	cd win-x86-bin; \
	find . -maxdepth 1 -name 'livecodeforfm-*.fmx' -print0 \
	    | xargs -0 tar --append --file=../mac-installer.tar; \
	cd ..; \
	cd win-x86_64-bin; \
	find . -maxdepth 1 -name 'livecodeforfm-*.fmx64' -print0 \
	    | xargs -0 tar --append --file=../mac-installer.tar; \
	cd ..; \
	bzip2 -c mac-installer.tar > mac-installer.tar.xz

distmac-extract:
	set -e; \
	tar -xvf mac-installer.tar.xz; \
	cp -r ${private_dir}/filemaker/solutions/LiveCodeForFM.fmp12 . ; \
	$(buildtool_command) --platform mac --stage fmpackage --debug; \
	$(buildtool_command) --platform win-x86 --stage fmpackage --debug; \
	$(buildtool_command) --platform win-x86_64 --stage fmpackage --debug; \
	$(buildtool_command) --platform universal --stage fmpackage --debug; \
	find . -maxdepth 1 -name 'LiveCodeForFM-Mac-*.fmp12' -print0 \
	    | xargs -0 zip -r LiveCodeForFM-Mac-Solution.zip; \
	find . -maxdepth 1 -name 'LiveCodeForFM-Win-x86-*.fmp12' -print0 \
	    | xargs -0 zip -r LiveCodeForFM-Win-x86-Solution.zip; \
	find . -maxdepth 1 -name 'LiveCodeForFM-Win-x86_64-*.fmp12' -print0 \
	    | xargs -0 zip -r LiveCodeForFM-Win-x86_64-Solution.zip; \
	find . -maxdepth 1 -name 'LiveCodeForFM-[1-9]*.fmp12' -print0 \
	    | xargs -0 zip -r LiveCodeForFM.zip; \
	find . -maxdepth 1 -name 'livecodeforfm-*.*' -print0 \
	    | xargs -0 zip -r LiveCodeForFM-All-Plugins.zip; \
	find . -maxdepth 1 -name 'livecodeforfm-*.fmplugin' -print0 \
	    | xargs -0 zip -r LiveCodeForFM-Mac-Plugin.zip; \
	find . -maxdepth 1 -name 'livecodeforfm-*.fmx' -print0 \
	    | xargs -0 zip -r LiveCodeForFM-Win-x86-Plugin.zip; \
	find . -maxdepth 1 -name 'livecodeforfm-*.fmx64' -print0 \
	    | xargs -0 zip -r LiveCodeForFM-Win-x86_64-Plugin.zip; \
	find . -maxdepth 1 -name 'LiveCodeForFM.fmp12' -print0 \
	    | xargs -0 zip -r LiveCodeForFM-Solution.zip


# Final installer creation for Mac
distmac-disk-%: distmac-bundle-%
	$(buildtool_command) --platform mac --stage disk --edition $*

distmac-upload: dist-upload
