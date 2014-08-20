###############################################################################
# Linux Application Makefile Template

# Make sure the environment makefile has been included.
ifeq ($(ARCH),)
	$(error Environment Makefile not included!)
endif

TYPE_DEFINES=
TYPE_INCLUDES=
TYPE_CCFLAGS=

include $(shell pwd)/$(dir $(lastword $(MAKEFILE_LIST)))/common.linux.makefile

###############################################################################
# Customizations

LIBS=$(CUSTOM_LIBS)
SHARED_LIBS=$(CUSTOM_SHARED_LIBS)
STATIC_LIBS=$(CUSTOM_STATIC_LIBS)
DYNAMIC_LIBS=$(CUSTOM_DYNAMIC_LIBS)

ifeq ($(ARCH),x86_64)
	DYNAMIC_LIBS+=stdc++
else
	STATIC_LIBS+=stdc++
endif

LDFLAGS=$(CUSTOM_LDFLAGS) $(addprefix -L,$(GLOBAL_LIBS)) $(addprefix -L,$(CUSTOM_INCLUDES)) -Xlinker -no-undefined $(addprefix -Xlinker --exclude-libs -Xlinker ,$(addsuffix .a,$(addprefix lib,$(STATIC_LIBS))))

TARGET_PATH=$(BUILD_DIR)/$(NAME)

$(TARGET_PATH): $(OBJECTS) $(DEPS)
	mkdir -p $(dir $(TARGET_PATH))
	$(CC) -fvisibility=hidden -o$(TARGET_PATH) $(LDFLAGS) $(OBJECTS) $(CUSTOM_OBJECTS) \
			-Wl,-Bstatic \
			-Wl,--start-group \
				$(addsuffix .a,$(addprefix $(PRODUCT_DIR)/lib,$(LIBS))) \
				$(addprefix -l,$(STATIC_LIBS)) \
			-Wl,--end-group \
			-Wl,-Bdynamic \
			$(addsuffix .so,$(addprefix $(PRODUCT_DIR)/,$(SHARED_LIBS))) \
			$(addprefix -l,$(DYNAMIC_LIBS))
ifneq ($(MODE),debug)
	cd $(BUILD_DIR) && \
		$(OBJCOPY) --only-keep-debug "$(NAME)" "$(NAME).dbg" && \
		$(STRIP) --strip-debug --strip-unneeded "$(NAME)" && \
		$(OBJCOPY) --add-gnu-debuglink="$(NAME).dbg" "$(NAME)"
endif

.PHONY: $(NAME)
$(NAME): $(TARGET_PATH)
