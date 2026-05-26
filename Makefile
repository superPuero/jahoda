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
CC_COMMON_DEBUG_FLAGS = $(CC_COMMON_FLAGS) -O0 -g -gcodeview
CC_WINDOWS_FLAGS = -D_GLFW_WIN32

#----------------------

# --- lib flags ---
LIB_COMMON_FLAGS = $(LIBRARY_DIRECTORIES)
LIB_WINDOWS_FLAGS = -lvulkan-1 -lgdi32
#-----------------

# --- sources ---
CORE_SRCS = $(wildcard src/core/*.c)
BASE_SRCS = $(wildcard src/base/*.c)
GFX_SRCS = $(wildcard src/gfx/*.c)
GLFW_SRCS = $(wildcard vendors/glfw/src/*.c)
TRUETYPE_SRC = $(wildcard vendors/stb_truetype/*.c)
FAST_OBJ_SRC = $(wildcard vendors/fast_obj/*.c)

SRCS = main.c $(CORE_SRCS) $(BASE_SRCS) $(GLFW_SRCS) $(GFX_SRCS) $(TRUETYPE_SRC) $(FAST_OBJ_SRC)
# ---------------

OBJS = $(SRCS:.c=.o)

# --- windows targets ---
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

# ----------------------

# todo: linux targets

# --- shared rules ---
%.o: %.c
	$(CC) $(CC_FLAGS) -c $< -o $@
# --------------------

SHADER_SRCS = $(wildcard shaders/*.vert) $(wildcard shaders/*.frag)
SHADER_SPVS = $(SHADER_SRCS:%=%.spv)

compile_shaders: $(SHADER_SPVS)

clean_shaders: 
	del /S /Q shaders\*.spv

%.frag.spv: %.frag
	glslc $< -o $@

%.vert.spv: %.vert
	glslc $< -o $@


