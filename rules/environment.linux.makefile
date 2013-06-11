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

BUILD_DIR=$(SOLUTION_DIR)/_build/linux/$(ARCH)/$(MODE)
CACHE_DIR=$(SOLUTION_DIR)/_cache/linux/$(ARCH)/$(MODE)/$(NAME)
PRODUCT_DIR=$(BUILD_DIR)

PREBUILT_LIB_DIR=$(SOLUTION_DIR)/prebuilt/lib/linux-$(ARCH)
