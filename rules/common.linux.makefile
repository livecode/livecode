# Common template Makefile for all types

DEFINES=$(CUSTOM_DEFINES) $(TYPE_DEFINES) _LINUX TARGET_PLATFORM_POSIX _FILE_OFFSET_BITS=64

GLOBAL_PACKAGES=\
	gtk+-2.0 \
	gtk+-unix-print-2.0 \
	gdk-2.0 \
	freetype2 \
	xft

GLOBAL_INCLUDES=\
	$(SOLUTION_DIR)/engine/include \
	$(SOLUTION_DIR)/libfoundation/include \
	$(SOLUTION_DIR)/lcidlc/include \
	$(SOLUTION_DIR)/libcore/include \
	$(SOLUTION_DIR)/libexternal/include \
	$(SOLUTION_DIR)/libgraphics/include \
	$(SOLUTION_DIR)/thirdparty/libiodbc/include \
	$(SOLUTION_DIR)/thirdparty/libjpeg/include \
	$(SOLUTION_DIR)/thirdparty/libmysql/include \
	$(SOLUTION_DIR)/thirdparty/libpcre/include \
	$(SOLUTION_DIR)/thirdparty/libpng/include \
	$(SOLUTION_DIR)/thirdparty/libgif/include \
	$(SOLUTION_DIR)/thirdparty/libpq/include \
	$(SOLUTION_DIR)/thirdparty/libsqlite/include \
	$(SOLUTION_DIR)/thirdparty/libxml/include \
	$(SOLUTION_DIR)/thirdparty/libxslt/include \
	$(SOLUTION_DIR)/thirdparty/libz/include \
	$(SOLUTION_DIR)/thirdparty/libzip/include \
	$(SOLUTION_DIR)/thirdparty/libopenssl/include \
	$(SOLUTION_DIR)/thirdparty/libcurl/include \
	$(SOLUTION_DIR)/thirdparty/libskia/include/core \
	$(SOLUTION_DIR)/thirdparty/libskia/include/config \
	$(SOLUTION_DIR)/thirdparty/libskia/include/effects \
	$(SOLUTION_DIR)/thirdparty/libskia/include/images \
	$(SOLUTION_DIR)/thirdparty/libskia/include/pathops \
	$(SOLUTION_DIR)/thirdparty/libskia/include/ports \
	$(SOLUTION_DIR)/thirdparty/libskia/include/utils \
	$(SOLUTION_DIR)/prebuilt/include
	
GLOBAL_LIBS=\
	$(PREBUILT_LIB_DIR) \
	$(SOLUTION_DIR)/prebuilt/lib/linux-$(ARCH)

ifeq ($(MODE),debug)
	DEFINES+=_DEBUG HAVE_VALGRIND
else
	DEFINES+=_RELEASE NDEBUG
endif

DEFINES += __LITTLE_ENDIAN__

PACKAGE_INCLUDES=$(shell pkg-config --silence-errors --cflags-only-I $(GLOBAL_PACKAGES))

# MDW-2014-11-20 [[ fix_valgrind_path]]
# avoids the libcairo compilation error
FALLBACK_INCLUDES=-I$(SOLUTION_DIR)/thirdparty/headers/linux/include -I$(SOLUTION_DIR)/thirdparty/headers/linux/include/cairo -I$(SOLUTION_DIR)/thirdparty/headers/linux/include/valgrind

INCLUDES=$(CUSTOM_INCLUDES) $(TYPE_INCLUDES) $(GLOBAL_INCLUDES)

ifeq ($(MODE),release)
	CCFLAGS=$(CUSTOM_CCFLAGS) $(TYPE_CCFLAGS) -O3 -fvisibility=hidden -g
else
	CCFLAGS=$(CUSTOM_CCFLAGS) $(TYPE_CCFLAGS) -g -fvisibility=hidden
endif

DEPS=$(addprefix $(BUILD_DIR)/, $(CUSTOM_DEPS))
SRC_OBJECTS=$(addprefix $(CACHE_DIR)/, $(addsuffix .o, $(basename $(SOURCES))))
OBJECTS=$(SRC_OBJECTS) $(CUSTOM_OBJECTS)

HDEPS:=$(SRC_OBJECTS:.o=.d)
-include $(HDEPS)

VPATH=./src $(SOURCE_DIRS) $(CACHE_DIR) $(BUILD_DIR)

$(CACHE_DIR)/%.o: %.cpp
	mkdir -p $(CACHE_DIR)/$(dir $*)
	$(CXX) $(CCFLAGS) $(addprefix -I,$(INCLUDES)) $(PACKAGE_INCLUDES) $(FALLBACK_INCLUDES) $(addprefix -D,$(DEFINES)) -MMD -MF $(patsubst %.o,%.d,$@) -c -o$(CACHE_DIR)/$*.o ./src/$*.cpp

$(CACHE_DIR)/%.o: %.c
	mkdir -p $(CACHE_DIR)/$(dir $*)
	$(CC) $(CCFLAGS) $(addprefix -I,$(INCLUDES)) $(PACKAGE_INCLUDES) $(FALLBACK_INCLUDES) $(addprefix -D,$(DEFINES)) -MMD -MF $(patsubst %.o,%.d,$@) -c -o$(CACHE_DIR)/$*.o ./src/$*.c
	
$(CACHE_DIR)/%.o: %.s
	mkdir -p $(CACHE_DIR)/$(dir $*)
	$(CC) $(CCFLAGS) $(addprefix -I,$(INCLUDES)) $(PACKAGE_INCLUDES) $(FALLBACK_INCLUDES) $(addprefix -D,$(DEFINES)) -c -o$(CACHE_DIR)/$*.o ./src/$*.s

clean:
	rm $(OBJECTS)
	rm $(TARGET_PATH)

