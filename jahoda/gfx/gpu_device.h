#ifndef jahoda_gpu_device
#define jahoda_gpu_device

#include "gpu_glfw.h"
#include "gpu_instance.h"
#include "gpu_physical_device.h"
#include "gpu_surface.h"

#define jahoda_gpu_invalid_queue_family (u32)(-1)

da_declare(VkQueueFamilyProperties, gpu_queue_family_properties_da);
da_declare(VkDeviceQueueCreateInfo, gpu_device_queue_ci_da);
da_declare(VkExtensionProperties, gpu_exntension_propertioes_da);

#define gpu_check_2(expr, device, arena) \
do { \
	VkResult val = (expr); \
	if (val < 0){\
		VkDeviceFaultCountsEXT counts = { .sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT };\
		(device)->get_device_fault_info((device)->handle, &counts, NULL);\
\
		VkDeviceFaultAddressInfoEXT *addrs  = arena_push_bytes(arena, counts.addressInfoCount + 1 * sizeof *addrs, 1, false);\
		VkDeviceFaultVendorInfoEXT  *vendor = arena_push_bytes(arena, counts.vendorInfoCount + 1 * sizeof *vendor, 1, false);\
\
		VkDeviceFaultInfoEXT info = {\
			.sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT,\
			.pAddressInfos = addrs,\
			.pVendorInfos  = vendor,\
			.pVendorBinaryData = NULL,\
		};\
		(device)->get_device_fault_info((device)->handle, &counts, &info);\
\
		info("fault: %s", info.description);\
\
		for each_index(i, counts.addressInfoCount)\
		{\
			info("  addr type=%s  addr=%llx  precision=%llu",\
				gpu_device_fault_address_type_to_cstr(addrs[i].addressType), addrs[i].reportedAddress, addrs[i].addressPrecision);\
		}\
		for each_index(i, counts.vendorInfoCount)\
		{\
			info("  vendor: %s code=%llu data=%llu", \
				vendor[i].description, vendor[i].vendorFaultCode, vendor[i].vendorFaultData);\
		}\
\
	}\
	dbg_verifyl(gpu_check_fail, val == VK_SUCCESS, "%s resulted in %s", #expr, gpu_result_to_string(val));\
} while(0)

typedef struct
{
	VkDevice handle;
	
	u32 queue_graphics_index;
	u32 queue_graphics_timestamp_valid_bits;
	u32 queue_transfer_index;
	u32 queue_compute_index;
	u32 queue_present_index;

	gpu_physical_device *pdevice;

	PFN_vkGetDescriptorSetLayoutSizeEXT get_descriptor_set_layout_size;
    PFN_vkGetDescriptorSetLayoutBindingOffsetEXT get_descriptor_set_layout_binding_offset;
    PFN_vkGetDescriptorEXT get_descriptor;
    PFN_vkCmdBindDescriptorBuffersEXT cmd_bind_descriptor_buffer;
    PFN_vkCmdSetDescriptorBufferOffsetsEXT cmd_set_descriptor_buffer_offset;

	PFN_vkGetDeviceFaultInfoEXT 		get_device_fault_info;
} gpu_device;

const char *gpu_device_fault_address_type_to_cstr(VkDeviceFaultAddressTypeEXT type);
gpu_device gpu_device_make(arena *temp_memory, gpu_physical_device *pdevice, gpu_surface *surface);
void gpu_device_release(gpu_device *device);
void gpu_device_wait_idle(gpu_device *device);

#endif