ifeq ($(OS),Windows_NT)
    PLATFORM = windows
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        PLATFORM = linux
    endif
    ifeq ($(UNAME_S),Darwin)
        PLATFORM = macos
    endif
endif

$(building for platform: $(PLATFORM))

ifndef VULKAN_SDK
	$(error VULKAN_SDK is not detected)
endif

CC = clang

BUILD_DIR = build

TARGET = jahoda

INCLUDE_DIRECTORIES = -I"$(VULKAN_SDK)/include" -I"vendors/glfw/include" -I"vendors/stb_truetype" -I"vendors/stb_image" -I.
LIB_DIRECTORIES = -Lpthreads -lws2_32
LIB_FLAGS =

CC_FLAGS = -std=c99 -Wall -Wextra $(INCLUDE_DIRECTORIES)
CC_RELEASE_FLAGS = -O3 -DNDEBUG
CC_DEBUG_FLAGS = -O0

include make/$(PLATFORM).mk
include jahoda/jahoda.mk
include vendors/vendors.mk

OBJS = $(SRCS:.c=.o)

basic_build: $(OBJS)
	$(CC) $(OBJS) $(CC_FLAGS) -o $(TARGET) $(LIB_DIRECTORIES) $(LIB_FLAGS)

%.o: %.c
	$(CC) $(CC_FLAGS) -c $< -o $@

