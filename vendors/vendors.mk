GLFW_SRCS = $(wildcard vendors/glfw/src/*.c)
GLFW_M_SRCS = $(wildcard vendors/glfw/src/*.m)
TRUETYPE_SRC = $(wildcard vendors/stb_truetype/*.c)
FAST_OBJ_SRC = $(wildcard vendors/fast_obj/*.c)
STB_IMAGE_SRC = $(wildcard vendors/stb_image/*.c)

SRCS += $(GLFW_SRCS) $(TRUETYPE_SRC) $(FAST_OBJ_SRC) $(STB_IMAGE_SRC)

ifeq ($(PLATFORM),windows)

CC_FLAGS += -D_GLFW_WIN32	

else ifeq ($(PLATFORM),linux)

CC_FLAGS += D_GLFW_X11

else ifeq ($(PLATFORM),macos)

M_SRCS = $(GLFW_M_SRCS)

CC_FLAGS += -D_GLFW_COCOA

%.o: %.m
	$(CC) $(CC_FLAGS) -c $< -o $@

M_OBJS = $(M_SRCS:.m=.o)

OBJS += $(M_OBJS)

endif