SHADER_SRCS = $(wildcard assets/shaders/*.vert) $(wildcard assets/shaders/*.frag) $(wildcard assets/shaders/*.comp)
SHADER_SPVS = $(SHADER_SRCS:%=%.spv)

compile_shaders: $(SHADER_SPVS)

%.frag.spv: %.frag
	glslc --target-env=vulkan1.2 $< -O -o $@

%.vert.spv: %.vert
	glslc --target-env=vulkan1.2 $< -O -o $@

%.comp.spv: %.comp
	glslc --target-env=vulkan1.2 $< -O -o $@