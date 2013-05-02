# Common environment variables

ifeq ($(MODE),)
	MODE=debug
endif

ifeq ($(EDITION),)
	SOLUTION_DIR=$(shell cat ../owner)/..
else
	SOLUTION_DIR=../livecode
endif

ifeq ($(shell gcc -dumpmachine),x86_64-linux-gnu)
	PLATFORM=linux-x64
else
	PLATFORM=linux
endif

BUILD_DIR=$(SOLUTION_DIR)/_build/$(PLATFORM)/$(MODE)
CACHE_DIR=$(SOLUTION_DIR)/_cache/$(PLATFORM)/$(MODE)/$(NAME)
PRODUCT_DIR=$(BUILD_DIR)
