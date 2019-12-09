#
# Main component makefile.
#
# This Makefile can be left empty. By default, it will take the sources in the 
# src/ directory, compile them and link them into lib(subdirectory_name).a 
# in the build directory. This behaviour is entirely configurable,
# please read the ESP-IDF documents if you need to do this.
#
COMPONENT_ADD_INCLUDEDIRS += include 

LIB_PATH := $(COMPONENT_PATH)/lib/libxBlufi.a

COMPONENT_ADD_LDFLAGS := $(LIB_PATH) \

COMPONENT_ADD_LINKER_DEPS := $(LIB_PATH)

