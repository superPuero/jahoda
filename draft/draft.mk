SRCS += draft/main.c

include make/core.mk

TARGET = draft_app

debug: CC_FLAGS += $(CC_DEBUG_FLAGS)
debug: basic_build

release: CC_FLAGS += $(CC_RELEASE_FLAGS)
release: basic_build