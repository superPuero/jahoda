#include "texture.h"

void texture_release(vk_device *device, texture *texture)
{
	vkDestroyImageView(device->handle, texture->view, NULL);
	vkDestroyImage(device->handle, texture->image, NULL);
}