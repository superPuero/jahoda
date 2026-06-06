SRCS += projects/ui_dev/main.c projects/ui_dev/env.c 
include make/core.mk

TARGET = ui_dev

debug: CC_FLAGS += $(CC_DEBUG_FLAGS)
debug: basic_build

release: CC_FLAGS += $(CC_RELEASE_FLAGS)
release: basic_build
