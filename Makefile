ifndef VULKAN_SDK
$(error VULKAN_SDK is not detected. Install the Vulkan SDK before running make)
endif

CC = clang

TARGET = jahoda

INCLUDE_DIRECTORIES = -I"$(VULKAN_SDK)/include" -I"vendors/glfw/include" -I"vendors/stb_truetype" -I.
LIBRARY_DIRECTORIES = -L"$(VULKAN_SDK)/lib" 

# --- compile flags ---

CC_COMMON_FLAGS = -std=c99 -Wall -Wextra $(INCLUDE_DIRECTORIES)
CC_COMMON_RELEASE_FLAGS = $(CC_COMMON_FLAGS) -O3 -DNDEBUG

# Note: -gcodeview is Windows specific. We will strip it out for the Linux debug target.
CC_COMMON_DEBUG_FLAGS = $(CC_COMMON_FLAGS) -O0 -g -gcodeview 

CC_WINDOWS_FLAGS = -D_GLFW_WIN32
CC_LINUX_FLAGS = -D_GLFW_X11
CC_MAC_FLAGS = -D_GLFW_COCOA

#----------------------

# --- lib flags ---
LIB_COMMON_FLAGS = $(LIBRARY_DIRECTORIES)
LIB_WINDOWS_FLAGS = -lvulkan-1 -lgdi32
LIB_LINUX_FLAGS = -lvulkan -lX11 -lpthread -ldl -lm
LIB_MAC_FLAGS = -lvulkan -framework Cocoa -framework IOKit -framework CoreVideo
#-----------------

# --- sources ---
CORE_SRCS = $(wildcard src/core/*.c)
BASE_SRCS = $(wildcard src/base/*.c)
GFX_SRCS = $(wildcard src/gfx/*.c)

# Assuming your glfw/src folder is pruned for the target OS, 
# or GLFW's internal #ifdefs are handling the compilation of unused platform files.
GLFW_SRCS = $(wildcard vendors/glfw/src/*.c) $(wildcard vendors/glfw/src/*.m)
TRUETYPE_SRC = $(wildcard vendors/stb_truetype/*.c)
FAST_OBJ_SRC = $(wildcard vendors/fast_obj/*.c)

SRCS = main.c $(CORE_SRCS) $(BASE_SRCS) $(GLFW_SRCS) $(GFX_SRCS) $(TRUETYPE_SRC) $(FAST_OBJ_SRC) 
# ---------------

OBJS = $(patsubst %.m, %.o, $(patsubst %.c, %.o, $(SRCS)))

# ==============================================================================
#                              WINDOWS TARGETS
# ==============================================================================
release_windows: CC_FLAGS = $(CC_COMMON_RELEASE_FLAGS) $(CC_WINDOWS_FLAGS)
release_windows: LIB_FLAGS = $(LIB_COMMON_FLAGS) $(LIB_WINDOWS_FLAGS)

debug_windows: CC_FLAGS = $(CC_COMMON_DEBUG_FLAGS) $(CC_WINDOWS_FLAGS)
debug_windows: LIB_FLAGS = $(LIB_COMMON_FLAGS) $(LIB_WINDOWS_FLAGS) -fuse-ld=lld -Wl,--pdb=

release_windows: $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIB_FLAGS)

debug_windows: $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIB_FLAGS)

clean_windows_all:
	del /S /Q *.o
	del /Q $(TARGET).exe *.pdb

clean_windows:  
	del /S /Q src\*.o
	del /Q *.o
	del /Q $(TARGET).exe *.pdb

clean_shaders_windows: 
	del /S /Q shaders\*.spv

# ==============================================================================
#                               LINUX TARGETS
# ==============================================================================

release_linux: CC_FLAGS = $(CC_COMMON_RELEASE_FLAGS) $(CC_LINUX_FLAGS)
release_linux: LIB_FLAGS = $(LIB_COMMON_FLAGS) $(LIB_LINUX_FLAGS)

# We override the flags manually here so we don't pass the Windows -gcodeview flag
debug_linux: CC_FLAGS = $(CC_COMMON_FLAGS) -O0 -g $(CC_LINUX_FLAGS)
debug_linux: LIB_FLAGS = $(LIB_COMMON_FLAGS) $(LIB_LINUX_FLAGS)

release_linux: $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIB_FLAGS)

debug_linux: $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIB_FLAGS)

clean_linux_all:
	find . -type f -name '*.o' -delete
	rm -f $(TARGET)

clean_linux:  
	rm -f src/core/*.o src/base/*.o src/gfx/*.o *.o
	rm -f $(TARGET)

clean_shaders_linux: 
	rm -f shaders/*.spv

# ==============================================================================
#                               MAC TARGETS
# ==============================================================================

release_macos: CC_FLAGS = $(CC_COMMON_RELEASE_FLAGS) $(CC_MAC_FLAGS)
release_macos: LIB_FLAGS = $(LIB_COMMON_FLAGS) $(LIB_MAC_FLAGS)

# We override the flags manually here so we don't pass the Windows -gcodeview flag
debug_macos: CC_FLAGS = $(CC_COMMON_FLAGS) -O0 -g $(CC_MAC_FLAGS)
debug_macos: LIB_FLAGS = $(LIB_COMMON_FLAGS) $(LIB_MAC_FLAGS)

release_macos: $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIB_FLAGS)

debug_macos: $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIB_FLAGS)

clean_macos_all:
	find . -type f -name '*.o' -delete
	rm -f $(TARGET)

clean_macos:  
	rm -f src/core/*.o src/base/*.o src/gfx/*.o *.o
	rm -f $(TARGET)

clean_shaders_macos: 
	rm -f shaders/*.spv


# ==============================================================================
#                               SHARED RULES
# ==============================================================================
%.o: %.c
	$(CC) $(CC_FLAGS) -c $< -o $@

%.o: %.m
	$(CC) $(CC_FLAGS) -m $< -o $@

SHADER_SRCS = $(wildcard shaders/*.vert) $(wildcard shaders/*.frag)
SHADER_SPVS = $(SHADER_SRCS:%=%.spv)

compile_shaders: $(SHADER_SPVS)

%.frag.spv: %.frag
	glslc $< -o $@

%.vert.spv: %.vert
	glslc $< -o $@