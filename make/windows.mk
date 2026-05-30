LIB_DIRECTORIES += -L"$(VULKAN_SDK)/lib" -L"$(CUDA_PATH)/lib/x64"
LIB_FLAGS += -lvulkan-1 -lgdi32 -lUser32 -lShell32 -lcudart

CC_FLAGS += -D_GLFW_WIN32
CC_DEBUG_FLAGS += -gcodeview

clear_all:
	del /S /Q *.o
	del /Q $(TARGET).exe *.pdb

clear:  
	del /S /Q src\*.o
	del /Q *.o
	del /Q $(TARGET).exe *.pdb

clear_shaders: 
	del /S /Q shaders\*.spv
