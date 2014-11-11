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

ifneq ($(ARCH),x86_64)
	STATIC_LIBS+=stdc++
endif

LDFLAGS_COMMON=$(addprefix -L,$(GLOBAL_LIBS)) $(addprefix -L,$(CUSTOM_INCLUDES))
LDFLAGS_LTO:=$(CUSTOM_LDFLAGS_LTO) $(CUSTOM_LDFLAGS) $(LDFLAGS_COMMON) -Xlinker -no-undefined $(addprefix -Xlinker --exclude-libs -Xlinker ,$(addsuffix .a,$(addprefix lib,$(STATIC_LIBS))))
LDFLAGS_FINAL:=$(CUSTOM_LDFLAGS_FINAL) $(CUSTOM_LDFLAGS) $(LDFLAGS_COMMON) -no-undefined $(addprefix --exclude-libs ,$(addsuffix .a,$(addprefix lib,$(STATIC_LIBS))))

TARGET_PATH=$(BUILD_DIR)/$(NAME)

ifeq ($(LD_IS_CC),1)
	# If we are using the compiler to link, we need to prefix the linker options
	LDFLAGS_FINAL:=$(addprefix -Xlinker ,$(LDFLAGS_FINAL))

	LINK_STEP1:=$(CXX) -fvisibility=hidden -o$(TARGET_PATH) $(LDFLAGS_LTO) $(LDFLAGS_FINAL) $(OBJECTS) $(CUSTOM_OBJECTS) \
			-Wl,-Bstatic \
			-Wl,--start-group \
				$(addsuffix .a,$(addprefix $(PRODUCT_DIR)/lib,$(LIBS))) \
				$(addprefix -l,$(STATIC_LIBS)) \
			-Wl,--end-group \
			-Wl,-Bdynamic \
			$(addsuffix .so,$(addprefix $(PRODUCT_DIR)/,$(SHARED_LIBS))) \
			$(addprefix -l,$(DYNAMIC_LIBS))
	LINK_STEP2:=
else
	LINK_STEP1:=$(CXX) -Wl,-relocatable -nodefaultlibs -fvisibility=hidden -o$(TARGET_PATH).rel $(LDFLAGS_LTO) $(OBJECTS) $(CUSTOM_OBJECTS) \
			-Wl,-Bstatic \
			-Wl,--start-group \
				$(addsuffix .a,$(addprefix $(PRODUCT_DIR)/lib,$(LIBS))) \
				$(addprefix -l,$(STATIC_LIBS)) \
				-lgcc -lgcc_eh \
			-Wl,--end-group
	LINK_STEP2:=$(LD) -o$(TARGET_PATH) $(LDFLAGS_FINAL) $(TARGET_PATH).rel \
			-Bdynamic \
			$(addsuffix .so,$(addprefix $(PRODUCT_DIR)/,$(SHARED_LIBS))) \
			$(addprefix -l,$(DYNAMIC_LIBS)) -lc
endif


$(TARGET_PATH): $(OBJECTS) $(DEPS)
	mkdir -p $(dir $(TARGET_PATH))
	$(LINK_STEP1)
	$(LINK_STEP2)

ifneq ($(MODE),debug)
	cd $(BUILD_DIR) && \
		$(OBJCOPY) --only-keep-debug "$(NAME)" "$(NAME).dbg" && \
		$(STRIP) --strip-debug --strip-unneeded "$(NAME)" && \
		$(OBJCOPY) --add-gnu-debuglink="$(NAME).dbg" "$(NAME)"
endif

.PHONY: $(NAME)
$(NAME): $(TARGET_PATH)

