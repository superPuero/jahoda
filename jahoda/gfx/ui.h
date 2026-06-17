#ifndef jahoda_ui
#define jahoda_ui

#include "gpu_context.h"
#include <jahoda/base/tick_info.h>
#include "font_manager.h"

typedef u64 ui_node_type;
typedef u64 ui_node_id;
typedef u64 ui_node_state;

#define ui_scratch_arena_size Kb(128)

#define ui_node_type_list\
	XX(text, { str content; vec3_f32 color;  })\
	XX(button, { node_text text; vec3_f32 color; vec2_f64 measured_size; })

#define ui_node_state_list\
	X(pressed)\
	X(clicked)

typedef enum
{
	#define XX(t, s) ui_node_type_##t,
	ui_node_type_list
	#undef XX
} ui_node_type_;

typedef enum
{
	#define X(s) ui_state_bit_##s,
	ui_node_state_list
	#undef X
} ui_node_state_bit_;

typedef enum
{
	#define X(s) ui_node_state_##s = 1 << ui_state_bit_##s,
	ui_node_state_list
	#undef X

	ui_node_state_end
} ui_node_state_;

// @explain: assuming 8 bits in byte, which i think i reasonable thing to assume in 2026 (x2) 
static_assert(ui_node_state_end < (sizeof(ui_node_state)  *8));

#define XX(name, content) typedef struct content node_##name;
ui_node_type_list
#undef XX

typedef struct
{
	vec2_f64 position;
	ui_node_id parent;
	ui_node_id children[32];
	ui_node_type type;
	#define XX(name, content) node_##name name;
	union {
		ui_node_type_list
	};
	#undef XX
	ui_node_state state;
} ui_node;

// @explain: (1 << 12)  *ht_max_occupancy = 4096  *0.75 = 3072, this is hard limit for amount of nodes
// s_ht_declare(jui_node, 12, jui_node_table);

typedef struct
{
	vec2_f64 mouse_pos;
	bool8 mouse_down;
	bool8 mouse_click;
} ui_state;		

da_declare(ui_node, ui_node_da);

typedef struct
{
	u32 max_node_count;	
	strv name;
	font_manager *fonts;
	font_id font_id;
} ui_params;

typedef struct
{
	arena *pf_arena;

	VkShaderModule vertex_shader;
	VkShaderModule fragment_shader;

	VkPipelineLayout pipeline_layout; 
	VkPipeline pipeline;

	VkRenderPass render_pass;
	VkDescriptorPool descriptor_pool;
	VkDescriptorSetLayout descriptor_set_layout;
	VkSampler font_sampler;
	VkBuffer text_vbo[jahoda_vk_frames_in_flight];
	VkDescriptorSet descriptor_sets[jahoda_vk_frames_in_flight];
	VkImage albedo_images[jahoda_vk_frames_in_flight];
    VkImageView albedo_views[jahoda_vk_frames_in_flight];
    VkFramebuffer framebuffers[jahoda_vk_frames_in_flight];
	
	char *text_vbo_ptrs[jahoda_vk_frames_in_flight];

	ui_node_da nodes;
	ui_node *current_node;
	ui_state state;
	font_manager *fonts;
	u32 font_id;
} ui;


void ui_refresh(ui *ctx, ui_state new_state);
void ui_end(ui *ui);

void ui_text(ui *ctx, vec3_f32 color, vec2_f64 position, const u8 *fmt, ...);
bool8 ui_button(ui *ctx, vec3_f32 color, vec3_f32 text_color, vec2_f64 position, const u8 *fmt, ...);

ui ui_make(arena *pf_arena, gpu_context *gpu, font_manager *fonts, font_id font_id);
void ui_release(ui *ctx, gpu_context *gpu);
void ui_record_draw(ui *ui, gpu_context *gpu, VkCommandBuffer cmd);

#endif