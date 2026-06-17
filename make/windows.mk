LIB_DIRECTORIES += -L"$(VULKAN_SDK)/lib"
LIB_FLAGS += -lvulkan-1 -lgdi32

CC_DEBUG_FLAGS += -g -gcodeview -fuse-ld=lld -Wl,--pdb=

clear_all:
	del /S /Q *.o
	del /Q $(TARGET).exe *.pdb

clear:  
	del /S /Q src\*.o
	del /Q *.o
	del /Q $(TARGET).exe *.pdb

clear_shaders: 
	del /S /Q *.spv
