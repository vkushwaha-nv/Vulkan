/*
* Copyright (C) 2019-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*
* Utility functions (uniform buffers, resize handling, updates)
*/

#include "raytracingVK.h"

/*
	Create the uniform buffer used to pass matrices to the ray tracing ray generation shader
*/
void VulkanExample::createUniformBuffer()
{
	for (auto& buffer : uniformBuffers) {
		VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer, sizeof(UniformData), &uniformData));
		VK_CHECK_RESULT(buffer.map());
	}
}

/*
	If the window has been resized, we need to recreate the storage image and it's descriptor
*/
void VulkanExample::handleResize()
{
	// Recreate image
	createStorageImage(swapChain.colorFormat, { width, height, 1 });
	// Update descriptors
	VkDescriptorImageInfo storageImageDescriptor{ VK_NULL_HANDLE, storageImage.view, VK_IMAGE_LAYOUT_GENERAL };
	for (auto i = 0; i < maxConcurrentFrames; i++) {
		VkWriteDescriptorSet resultImageWrite = vks::initializers::writeDescriptorSet(descriptorSets[i], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &storageImageDescriptor);
		vkUpdateDescriptorSets(device, 1, &resultImageWrite, 0, VK_NULL_HANDLE);
	}
	resized = false;
}

void VulkanExample::updateUniformBuffers()
{
	uniformData.projInverse = glm::inverse(camera.matrices.perspective);
	uniformData.viewInverse = glm::inverse(camera.matrices.view);
	uniformData.lightPos = glm::vec4(cos(glm::radians(timer * 360.0f)) * 40.0f, -20.0f + sin(glm::radians(timer * 360.0f)) * 20.0f, 25.0f + sin(glm::radians(timer * 360.0f)) * 5.0f, 0.0f);
	// Pass the vertex size to the shader for unpacking vertices
	uniformData.vertexSize = sizeof(vkglTF::Vertex);
	memcpy(uniformBuffers[currentBuffer].mapped, &uniformData, sizeof(uniformData));
}

