CORE_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

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

$(info building for platform: $(PLATFORM))

INCLUDE_DIRECTORIES =
LIB_DIRECTORIES = 
LIB_FLAGS = -lpthread

CC = clang

CC_FLAGS = -std=c99 -Wall -Wextra -mavx2 -mfma $(INCLUDE_DIRECTORIES) -fblocks
CC_RELEASE_FLAGS = -O3 -DNDEBUG -march=native
CC_DEBUG_FLAGS = -O0 

OBJS += $(SRCS:.c=.o)

include $(CORE_DIR)$(PLATFORM).mk
include $(CORE_DIR)../jahoda/jahoda.mk

basic_build: $(OBJS)
	$(CC) $(OBJS) $(CC_FLAGS) -o $(TARGET) $(LIB_DIRECTORIES) $(LIB_FLAGS)
  
%.o: %.c
	$(CC) $(CC_FLAGS) -c $< -o $@

