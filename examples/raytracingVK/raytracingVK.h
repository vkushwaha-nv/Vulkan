/*
* Copyright (C) 2019-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include "VulkanRaytracingSample.h"
#include "VulkanglTFModel.h"

class VulkanExample : public VulkanRaytracingSample
{
public:
	AccelerationStructure bottomLevelAS{};
	AccelerationStructure topLevelAS{};

	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};
	struct ShaderBindingTables {
		ShaderBindingTable raygen;
		ShaderBindingTable miss;
		ShaderBindingTable hit;
	} shaderBindingTables;

	struct UniformData {
		glm::mat4 viewInverse;
		glm::mat4 projInverse;
		glm::vec4 lightPos;
		int32_t vertexSize{ 0 };
	} uniformData;
	std::array<vks::Buffer, maxConcurrentFrames> uniformBuffers;

	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
	std::array<VkDescriptorSet, maxConcurrentFrames> descriptorSets{};

	vkglTF::Model scene;

	VulkanExample();
	~VulkanExample();

	// Raytracing functions (implemented in raytracingVK_rt.cpp)
	void createBottomLevelAccelerationStructure();
	void createTopLevelAccelerationStructure();
	void createShaderBindingTables();
	void createRayTracingPipeline();
	void getEnabledFeatures();

	// Non-raytracing functions (implemented in raytracingVK.cpp)
	void createDescriptorSets();
	void prepare();
	void buildCommandBuffer();
	virtual void render();

	// Utility functions (implemented in raytracingVK_util.cpp)
	void createUniformBuffer();
	void handleResize();
	void updateUniformBuffers();
};

