VENDORS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

GLFW_SRCS = $(wildcard $(VENDORS_DIR)glfw/src/*.c)
GLFW_M_SRCS = $(wildcard $(VENDORS_DIR)glfw/src/*.m)
TRUETYPE_SRC = $(wildcard $(VENDORS_DIR)stb_truetype/*.c)
CGLTF_SRC = $(wildcard $(VENDORS_DIR)cgltf/*.c)
STB_IMAGE_SRC = $(wildcard $(VENDORS_DIR)stb_image/*.c)

SRCS += $(GLFW_SRCS) $(TRUETYPE_SRC) $(FAST_OBJ_SRC) $(STB_IMAGE_SRC) $(CGLTF_SRC)

INCLUDE_DIRECTORIES += 	-I"$(VENDORS_DIR)glfw/include"\
						-I"$(VENDORS_DIR)stb_truetype"\
						-I"$(VENDORS_DIR)stb_image"\
						-I"$(VENDORS_DIR)cgltf"\
						-I.$(VENDORS_DIR)

ifeq ($(PLATFORM),windows)

CC_FLAGS += -D_GLFW_WIN32	

else ifeq ($(PLATFORM),linux)

CC_FLAGS += -D_GLFW_X11

else ifeq ($(PLATFORM),macos)

M_SRCS = $(GLFW_M_SRCS)

CC_FLAGS += -D_GLFW_COCOA

%.o: %.m
	$(CC) $(CC_FLAGS) -c $< -o $@

M_OBJS = $(M_SRCS:.m=.o)

OBJS += $(M_OBJS)

endif