PLATFORM_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
WIN_TARGET_DIR := $(subst /,\,$(PLATFORM_DIR)..)

LIB_DIRECTORIES += -L"$(VULKAN_SDK)/lib"
LIB_FLAGS += -lvulkan-1 -lgdi32 -lws2_32

CC_DEBUG_FLAGS += -g -gcodeview -fuse-ld=lld -Wl,--pdb=

ifeq ($(nocon), true)
	CC_FLAGS += -mwindows
endif


clear:
	del /S /Q $(WIN_TARGET_DIR)\*.o
	del /S /Q $(WIN_TARGET_DIR)\*.exe
	del /S /Q $(WIN_TARGET_DIR)\*.pdb

clear_shaders: 
	del /S /Q *.spv