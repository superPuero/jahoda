SRCS += main.c\
		jgfx.c\
		cmd_line_args.c\
		depth_pass.c\
		gbuffer_pass.c\
		deferred_light_pass.c\
		transform.c

include ../make/core.mk
include assets/shaders/compile_shaders.mk

TARGET = jgfx

ifeq ($(nocon), true)
	ifeq ($(PLATFORM), window)
		CC_FLAGS += -mwindows
	endif
	CC_FLAGS += -Djahoda_nocon
endif

debug: CC_FLAGS += $(CC_DEBUG_FLAGS)
debug: basic_build

release: CC_FLAGS += $(CC_RELEASE_FLAGS)
release: basic_build