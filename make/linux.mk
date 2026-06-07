LIB_DIRECTORIES += -L"$(VULKAN_SDK)/lib"
LIB_FLAGS += -lvulkan -lX11 -lpthread -ldl -lm

clear_all:
	find . -type f -name '*.o' -delete
	rm -f $(TARGET)

clear:  
	rm -f src/core/*.o src/base/*.o src/gfx/*.o *.o
	rm -f $(TARGET)

clear_shaders: 
	rm -f *.spv