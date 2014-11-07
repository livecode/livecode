###############################################################################
# Linux Shared Library Makefile Template

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

LIBS=$(CUSTOM_LIBS)
STATIC_LIBS=$(CUSTOM_STATIC_LIBS)
DYNAMIC_LIBS=$(CUSTOM_DYNAMIC_LIBS)

ifneq ($(ARCH),x86_64)
	STATIC_LIBS+=stdc++
endif

LDFLAGS=$(CUSTOM_LDFLAGS) -shared $(addprefix -Xlinker --exclude-libs -Xlinker ,$(addsuffix .a,$(addprefix lib,$(STATIC_LIBS)))) -Xlinker -no-undefined -static-libgcc

TARGET_PATH=$(BUILD_DIR)/$(NAME).so

$(TARGET_PATH): $(OBJECTS) $(DEPS)
	mkdir -p $(dir $(TARGET_PATH))
	$(CC) -fvisibility=hidden -o$(TARGET_PATH) $(LDFLAGS) $(OBJECTS) $(addsuffix .a,$(addprefix $(PRODUCT_DIR)/lib,$(LIBS))) -Wl,-Bstatic $(addprefix -l,$(STATIC_LIBS)) -Wl,-Bdynamic $(addprefix -l,$(DYNAMIC_LIBS))
	cd $(BUILD_DIR) && \
		$(OBJCOPY) --only-keep-debug "$(NAME).so" "$(NAME).so.dbg" && \
		$(STRIP) --strip-debug --strip-unneeded "$(NAME).so" && \
		$(OBJCOPY) --add-gnu-debuglink="$(NAME).so.dbg" "$(NAME).so"
		
.PHONY: $(NAME)
$(NAME): $(TARGET_PATH)
