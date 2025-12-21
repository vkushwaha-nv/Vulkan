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
	uniformData.vertexSize = useTestGeometry ? static_cast<int32_t>(testVertexStride) : sizeof(vkglTF::Vertex);
	memcpy(uniformBuffers[currentBuffer].mapped, &uniformData, sizeof(uniformData));
}

/*
	Create test geometry - a 5-pointed star made of triangles
*/
void VulkanExample::createTestGeometry()
{
	// Create a 5-pointed star made of 10 triangles
	// Each triangle goes from center to two adjacent outer points
	const float outerRadius = 2.0f;
	const float innerRadius = 0.8f;
	const float PI = 3.14159265359f;

	// Star has 10 points alternating between outer and inner radius
	std::vector<glm::vec3> starPoints(10);
	for (int i = 0; i < 10; i++) {
		float angle = (i * 36.0f - 90.0f) * PI / 180.0f;  // Start from top
		float radius = (i % 2 == 0) ? outerRadius : innerRadius;
		starPoints[i] = glm::vec3(cos(angle) * radius, sin(angle) * radius, 0.0f);
	}

	// Create vertices with position, normal, uv, color (matching vkglTF::Vertex layout)
	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv;
		glm::vec4 color;
		glm::vec4 _pad0;
		glm::vec4 _pad1;
	};

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	// Center vertex
	Vertex center{};
	center.pos = glm::vec3(0.0f, 0.0f, 0.0f);
	center.normal = glm::vec3(0.0f, 0.0f, 1.0f);
	center.uv = glm::vec2(0.5f, 0.5f);
	center.color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);  // Yellow center
	vertices.push_back(center);

	// Add star points as vertices
	for (int i = 0; i < 10; i++) {
		Vertex v{};
		v.pos = starPoints[i];
		v.normal = glm::vec3(0.0f, 0.0f, 1.0f);
		v.uv = glm::vec2(0.0f, 0.0f);
		// Alternate colors: outer points are red, inner points are orange
		v.color = (i % 2 == 0) ? glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) : glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
		vertices.push_back(v);
	}

	// Create 10 triangles (center + two adjacent points each)
	for (int i = 0; i < 10; i++) {
		indices.push_back(0);  // Center
		indices.push_back(1 + i);  // Current point
		indices.push_back(1 + ((i + 1) % 10));  // Next point
	}

	testVertexCount = static_cast<uint32_t>(vertices.size());
	testIndexCount = static_cast<uint32_t>(indices.size());
	testVertexStride = sizeof(Vertex);

	// Create vertex buffer
	VK_CHECK_RESULT(vulkanDevice->createBuffer(
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&testVertexBuffer,
		vertices.size() * sizeof(Vertex),
		vertices.data()));

	// Create index buffer
	VK_CHECK_RESULT(vulkanDevice->createBuffer(
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&testIndexBuffer,
		indices.size() * sizeof(uint32_t),
		indices.data()));
}

