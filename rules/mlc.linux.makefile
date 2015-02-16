#                                                                 -*-Makefile-*-

################################################################
# MLC
#

# Should have been set to a file containing a list of MLC files to build
MLC_LIST ?= $(NAME)-modules.list

SRC_DIR ?= ./src
MLC_SRC_DIR = $(SRC_DIR)/_mlc

LC_COMPILE ?= $(shell PATH=$(BUILD_DIR):$(PATH) \
	              which lc-compile 2>/dev/null || \
	              echo "lc-compile")

# Remove comments and '!' prefixes to filenames (the flag indicates
# that they shouldn't be used when bootstrapping, but since we're
# building normally we can ignore that flag now)
MLC_SOURCES = $(shell cat $(MLC_LIST) | grep -v '^\#' | sed -e's:^!::')

MLC_BUILT_SOURCES = $(patsubst %.mlc,_mlc/%.c,$(MLC_SOURCES))

MLC_STAMP ?= $(MLC_SRC_DIR)/stamp-mlc-$(NAME)

$(MLC_STAMP): $(MLC_SOURCES) $(MLC_LIST) $(LC_COMPILE)
	mkdir -p $(MODULE_DIR) $(MLC_SRC_DIR)
	@for f in $(MLC_SOURCES); do \
	    mlcfile=$(SRC_DIR)/$$f ; \
	    cfile=$(MLC_SRC_DIR)/`echo $$f | sed -e's:mlc$$:c:'` ; \
	    echo "$(LC_COMPILE) --modulepath $(MODULE_DIR) --outputc $$cfile $$mlcfile" $(_PRINT_RULE); \
	    $(LC_COMPILE) --modulepath $(MODULE_DIR) --outputc $$cfile $$mlcfile \
	        || exit $$? ; \
	done
	touch $@

$(MLC_BUILT_SOURCES): $(MLC_STAMP)

# Add MLC-derived files to list of source files to compile
SOURCES += $(MLC_BUILT_SOURCES)

clean: mlc-clean
mlc-clean:
	-rm -rf $(MLC_SRC_DIR)

.PHONY: mlc-clean

################################################################
