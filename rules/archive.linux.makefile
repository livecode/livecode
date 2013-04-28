###############################################################################
# Linux Static Library Makefile Template

TYPE_DEFINES=
TYPE_INCLUDES=
TYPE_CCFLAGS=-fPIC

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
