SRCS += projects/cuda_test/main.c projects/cuda_test/env.c 

include make/core.mk

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

# yet again windows shows everyone who is dumbes kid in the room
VCVARS = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

CC = call $(VCVARS) >nul && clang -target x86_64-pc-windows-msvc
NVCC = call $(VCVARS) >nul && nvcc

CUDA_SRCS = $(wildcard projects/cuda_test/cuda*.cu)
CUDA_OBJS = $(CUDA_SRCS:.cu=.o)

OBJS += $(CUDA_OBJS)

nvcc_build: $(OBJS)
	$(NVCC) $(NVCC_FLAGS) $(OBJS) -o $(TARGET) $(LIB_DIRECTORIES) $(LIB_FLAGS)

release: NVCC_FLAGS += -O3
release: nvcc_build

debug: NVCC_FLAGS += -O0
debug: nvcc_build

%.o: %.cu
	$(NVCC) $(NVCC_FLAGS) -c $< -o $@
