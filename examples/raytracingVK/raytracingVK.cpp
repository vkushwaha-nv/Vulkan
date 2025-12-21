/*
* Copyright (C) 2019-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*
* Non-raytracing functions (general Vulkan setup, descriptors, uniforms, rendering)
*/

#include "raytracingVK.h"

VulkanExample::VulkanExample() : VulkanRaytracingSample()
{
	title = "Ray tracing VK app";
	width = 2560;
	height = 1440;
	useTestGeometry = false;
	timerSpeed *= 0.5f;
	camera.rotationSpeed *= 0.25f;
	camera.type = Camera::CameraType::lookat;
	camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 512.0f);
	camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
	camera.setTranslation(glm::vec3(0.0f, 0.0f, -8.0f));
	enableExtensions();
}

VulkanExample::~VulkanExample()
{
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	deleteStorageImage();
	deleteAccelerationStructure(bottomLevelAS);
	deleteAccelerationStructure(topLevelAS);
	shaderBindingTables.raygen.destroy();
	shaderBindingTables.miss.destroy();
	shaderBindingTables.hit.destroy();
	for (auto& buffer : uniformBuffers) {
		buffer.destroy();
	}
	if (useTestGeometry) {
		testVertexBuffer.destroy();
		testIndexBuffer.destroy();
	} else {
		delete scene;
	}
}

/*
	Create the descriptor sets used for the ray tracing dispatch
*/
void VulkanExample::createDescriptorSets()
{
	std::vector<VkDescriptorPoolSize> poolSizes = {
		{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, maxConcurrentFrames },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxConcurrentFrames },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxConcurrentFrames },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxConcurrentFrames * 2 }
	};
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, maxConcurrentFrames);
	VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool));

	// Sets per frame, just like the buffers themselves
	// Acceleration structure, vertex and index buffers and images do not need to be duplicated per frame, we use the same for each descriptor to keep things simple
	VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
	for (auto i = 0; i < maxConcurrentFrames; i++) {
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i]));

		// The fragment shader needs access to the ray tracing acceleration structure, so we pass it as a descriptor

		VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
		descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
		descriptorAccelerationStructureInfo.pAccelerationStructures = &topLevelAS.handle;

		VkWriteDescriptorSet accelerationStructureWrite{};
		accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		// The specialized acceleration structure descriptor has to be chained
		accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
		accelerationStructureWrite.dstSet = descriptorSets[i];
		accelerationStructureWrite.dstBinding = 0;
		accelerationStructureWrite.descriptorCount = 1;
		accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

		VkDescriptorImageInfo storageImageDescriptor{ VK_NULL_HANDLE, storageImage.view, VK_IMAGE_LAYOUT_GENERAL };
		VkDescriptorBufferInfo vertexBufferDescriptor{ useTestGeometry ? testVertexBuffer.buffer : scene->vertices.buffer, 0, VK_WHOLE_SIZE };
		VkDescriptorBufferInfo indexBufferDescriptor{ useTestGeometry ? testIndexBuffer.buffer : scene->indices.buffer, 0, VK_WHOLE_SIZE };

		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			// Binding 0: Top level acceleration structure
			accelerationStructureWrite,
			// Binding 1: Ray tracing result image
			vks::initializers::writeDescriptorSet(descriptorSets[i], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &storageImageDescriptor),
			// Binding 2: Uniform data
			vks::initializers::writeDescriptorSet(descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &uniformBuffers[i].descriptor),
			// Binding 3: Scene vertex buffer
			vks::initializers::writeDescriptorSet(descriptorSets[i], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, &vertexBufferDescriptor),
			// Binding 4: Scene index buffer
			vks::initializers::writeDescriptorSet(descriptorSets[i], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, &indexBufferDescriptor),
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
	}
}

void VulkanExample::prepare()
{
	VulkanRaytracingSample::prepare();

	// Create the acceleration structures used to render the ray traced scene
	createBottomLevelAccelerationStructure();
	createTopLevelAccelerationStructure();

	createStorageImage(swapChain.colorFormat, { width, height, 1 });
	createUniformBuffer();
	createRayTracingPipeline();
	createShaderBindingTables();
	createDescriptorSets();
	prepared = true;
}

void VulkanExample::buildCommandBuffer()
{
	if (resized)
	{
		handleResize();
	}

	VkCommandBuffer cmdBuffer = drawCmdBuffers[currentBuffer];
	
	VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

	VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout, 0, 1, &descriptorSets[currentBuffer], 0, 0);

	/*
		Dispatch the ray tracing commands
	*/
	VkStridedDeviceAddressRegionKHR emptySbtEntry = {};
	vkCmdTraceRaysKHR(
		cmdBuffer,
		&shaderBindingTables.raygen.stridedDeviceAddressRegion,
		&shaderBindingTables.miss.stridedDeviceAddressRegion,
		&shaderBindingTables.hit.stridedDeviceAddressRegion,
		&emptySbtEntry,
		width,
		height,
		1);

	/*
		Copy ray tracing output to swap chain image
	*/

	// Prepare current swap chain image as transfer destination
	vks::tools::setImageLayout(
		cmdBuffer,
		swapChain.images[currentImageIndex],
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		subresourceRange);

	// Prepare ray tracing output image as transfer source
	vks::tools::setImageLayout(
		cmdBuffer,
		storageImage.image,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		subresourceRange);

	VkImageCopy copyRegion{};
	copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.srcOffset = { 0, 0, 0 };
	copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.dstOffset = { 0, 0, 0 };
	copyRegion.extent = { width, height, 1 };
	vkCmdCopyImage(cmdBuffer, storageImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapChain.images[currentImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	// Transition swap chain image back for presentation
	vks::tools::setImageLayout(
		cmdBuffer,
		swapChain.images[currentImageIndex],
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		subresourceRange);

	// Transition ray tracing output image back to general layout
	vks::tools::setImageLayout(
		cmdBuffer,
		storageImage.image,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL,
		subresourceRange);

	drawUI(cmdBuffer, frameBuffers[currentImageIndex]);

	VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
}

void VulkanExample::render()
{
	if (!prepared)
		return;
	VulkanExampleBase::prepareFrame();
	updateUniformBuffers();
	buildCommandBuffer();
	VulkanExampleBase::submitFrame();
}

VULKAN_EXAMPLE_MAIN()
