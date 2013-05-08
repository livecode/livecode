###############################################################################
# Linux Static Library Makefile Template

# Make sure the environment makefile has been included.
ifeq ($(ARCH),)
    $(error Environment Makefile not included!)
endif

TYPE_DEFINES=
TYPE_INCLUDES=
TYPE_CCFLAGS=

ifeq ($(ARCH),x86_64)
    TYPE_CCFLAGS=-fPIC
endif

include $(shell pwd)/$(dir $(lastword $(MAKEFILE_LIST)))/common.linux.makefile

###############################################################################
# Customizations

TARGET_PATH=$(BUILD_DIR)/$(NAME).a

$(TARGET_PATH): $(OBJECTS) $(DEPS)
	mkdir -p $(dir $(TARGET_PATH))
	ar rc $(TARGET_PATH) $(OBJECTS)

.PHONY: $(NAME)
$(NAME): $(TARGET_PATH)
	#
