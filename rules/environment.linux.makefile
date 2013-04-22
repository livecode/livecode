# Common environment variables

ifeq ($(MODE),)
	MODE=debug
endif

ifeq ($(EDITION),)
	SOLUTION_DIR=$(shell cat ../owner)/..
else
	SOLUTION_DIR=../livecode
endif

BUILD_DIR=$(SOLUTION_DIR)/_build/linux/$(MODE)
CACHE_DIR=$(SOLUTION_DIR)/_cache/linux/$(MODE)/$(NAME)
PRODUCT_DIR=$(BUILD_DIR)
