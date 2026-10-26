JAHODA_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

CORE_SRCS = $(wildcard  $(JAHODA_DIR)core/*.c)
BASE_SRCS = $(wildcard  $(JAHODA_DIR)base/*.c)
META_SRCS = $(wildcard  $(JAHODA_DIR)meta/*.c)

ifeq ($(gfx), true)
	GFX_SRCS = $(wildcard  $(JAHODA_DIR)gfx/*.c)
    include $(JAHODA_DIR)gfx/vendors/vendors.mk
endif

SRCS += $(CORE_SRCS) $(BASE_SRCS) $(GFX_SRCS) $(META_SRCS)

INCLUDE_DIRECTORIES += -I"$(JAHODA_DIR)../"