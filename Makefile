# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ../..

# FLAGS will be passed to both the C and C++ compiler
FLAGS +=
CFLAGS += 
CXXFLAGS += -DVERSION=\"3.4\" -Isrc/resid
ifdef DEBUG
CXXFLAGS += -DDEBUG_SID
endif

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
LDFLAGS +=

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp)
RESID_SRCS = dac.cc envelope.cc extfilt.cc filter.cc pot.cc sid.cc version.cc voice.cc wave.cc
SOURCES += $(patsubst %,src/resid/%,$(RESID_SRCS))

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk
