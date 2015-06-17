# Common environment variables

ifeq ($(MODE),)
    MODE=debug
endif

ifeq ($(EDITION),)
    SOLUTION_DIR=$(shell cat ../owner)/..
else
    SOLUTION_DIR=../livecode
endif

ifeq ($(ARCH),)
    GETCONF_LONG_BIT=$(shell getconf LONG_BIT)
    ifeq ($(GETCONF_LONG_BIT),32)
        ARCH=i386
	else
        ifeq ($(GETCONF_LONG_BIT),64)
            ARCH=x86_64
        endif
    endif
endif

ifeq ($(CC),)
	CC:=gcc
endif

ifeq ($(origin LD), default)
	LD:=$(CC)
	LD_IS_CC:=1
endif

BUILD_DIR=$(SOLUTION_DIR)/_build/linux/$(ARCH)/$(MODE)
CACHE_DIR=$(SOLUTION_DIR)/_cache/linux/$(ARCH)/$(MODE)/$(NAME)
MODULE_DIR=$(BUILD_DIR)/modules
PRODUCT_DIR=$(BUILD_DIR)

PREBUILT_LIB_DIR=$(SOLUTION_DIR)/prebuilt/lib/linux/$(ARCH)

OBJCOPY ?= objcopy
STRIP ?= strip
AR ?= ar

LC_COMPILE ?= $(shell PATH=$(BUILD_DIR):$(PATH) \
	              which lc-compile 2>/dev/null || \
	              echo "lc-compile")

LC_RUN ?= $(shell PATH=$(BUILD_DIR):$(PATH) \
	          which lc-run 2>/dev/null || \
	          echo "lc-run")

# Sometimes it's useful to print actions that a make rule is
# performing as if they're being run by make.  In that case, use the
# $(_PRINT_RULE) macro, e.g.
#
# mytarget:
#         @for foo in $(LIST); do \
#           echo "$(TOOL) $$foo" $(_PRINT_RULE); \
#           $(TOOL) $$foo || exit $?; \
#         done
ifneq (,$(findstring s,$(MAKEFLAGS)))
	_PRINT_RULE = > /dev/null
endif
