SRCS += $(wildcard projects/ml_dev/*.c)

include make/core.mk

TARGET = ml_dev

debug: CC_FLAGS += $(CC_DEBUG_FLAGS)
debug: basic_build

release: CC_FLAGS += $(CC_RELEASE_FLAGS)
release: basic_build
