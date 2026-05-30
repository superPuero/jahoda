SRCS += projects/cuda_test/main.c

include make/make/core.mk

TARGET = cuda_test

ifneq ($(PLATFORM),windows)
	$(error currently windows only)
endif

ifndef CUDA_PATH
	$(error CUDA_PATH is not detected)
endif

LIBRARY_DIRECTORIES += -L"$(CUDA_PATH)/lib/x64"
LIB_FLAGS += -lUser32 -lShell32 -lcudart
INCLUDE_DIRECTORIES += -I"$(CUDA_PATH)/include"

CC = clang -target x86_64-pc-windows-msvc
NVCC = nvcc
NVCC_FLAGS = -O3 -ccbin "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64"

CUDA_SRCS = $(wildcard pojects/cuda_test/cuda*.cu)
CUDA_OBJS = $(CUDA_SRCS:.cu=.o)

OBJS += $(CUDA_OBJS)

nvcc_build: $(OBJS)
	$(NVCC) $(NVCC_FLAGS) $(OBJS) -o $(TARGET) $(LIB_DIRECTORIES) $(LIB_FLAGS)

release: NVCC_FLAGS += CC_RELEASE_FLAGS
release: nvcc_build

debug: NVCC_FLAGS += CC_DEBUG_FLAGS
debug: nvcc_build

%.o: %.cu
	$(NVCC) $(NVCC_FLAGS) -c $< -o $@
