SHADER_SRCS = $(wildcard assets/shaders/*.vert) $(wildcard assets/shaders/*.frag)
SHADER_SPVS = $(SHADER_SRCS:%=%.spv)

compile_shaders: $(SHADER_SPVS)

%.frag.spv: %.frag
	glslc $< -o $@

%.vert.spv: %.vert
	glslc $< -o $@