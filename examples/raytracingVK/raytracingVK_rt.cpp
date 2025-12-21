/*
* Copyright (C) 2019-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*
* Raytracing-specific functions
*/

#include "raytracingVK.h"

/*
	Create the bottom level acceleration structure contains the scene's actual geometry (vertices, triangles)
*/
void VulkanExample::createBottomLevelAccelerationStructure()
{
	VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
	VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
	uint32_t numTriangles = 0;
	uint32_t maxVertex = 0;
	VkDeviceSize vertexStride = 0;

	if (useTestGeometry) {
		createTestGeometry();
		vertexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(testVertexBuffer.buffer);
		indexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(testIndexBuffer.buffer);
		numTriangles = testIndexCount / 3;
		maxVertex = testVertexCount - 1;
		vertexStride = testVertexStride;
	} else {
		// Load model from file
		vkglTF::memoryPropertyFlags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
		scene = new vkglTF::Model();
		scene->loadFromFile(getAssetPath() + "models/teapot.gltf", vulkanDevice, queue, glTFLoadingFlags);

		vertexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(scene->vertices.buffer);
		indexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(scene->indices.buffer);
		numTriangles = static_cast<uint32_t>(scene->indices.count) / 3;
		maxVertex = scene->vertices.count - 1;
		vertexStride = sizeof(vkglTF::Vertex);
	}

	// Build
	VkAccelerationStructureGeometryKHR accelerationStructureGeometry = vks::initializers::accelerationStructureGeometryKHR();
	accelerationStructureGeometry.flags = 0; // Non-opaque: any-hit shader will be called 
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR; //vktodo try aabb
	accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
	accelerationStructureGeometry.geometry.triangles.maxVertex = maxVertex;
	accelerationStructureGeometry.geometry.triangles.vertexStride = vertexStride;
	accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;
	accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
	accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;

	// Get size info
	VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = vks::initializers::accelerationStructureBuildGeometryInfoKHR();
	accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationStructureBuildGeometryInfo.geometryCount = 1;
	accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = vks::initializers::accelerationStructureBuildSizesInfoKHR();
	vkGetAccelerationStructureBuildSizesKHR(
		device,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&accelerationStructureBuildGeometryInfo,
		&numTriangles,
		&accelerationStructureBuildSizesInfo);

	createAccelerationStructure(bottomLevelAS, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, accelerationStructureBuildSizesInfo);

	// Create a small scratch buffer used during build of the bottom level acceleration structure
	ScratchBuffer scratchBuffer = createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

	VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo = vks::initializers::accelerationStructureBuildGeometryInfoKHR();
	accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	accelerationBuildGeometryInfo.dstAccelerationStructure = bottomLevelAS.handle;
	accelerationBuildGeometryInfo.geometryCount = 1;
	accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
	accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
	accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
	accelerationStructureBuildRangeInfo.primitiveOffset = 0;
	accelerationStructureBuildRangeInfo.firstVertex = 0;
	accelerationStructureBuildRangeInfo.transformOffset = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

	// Build the acceleration structure on the device via a one-time command buffer submission
	// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
	VkCommandBuffer commandBuffer = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	vkCmdBuildAccelerationStructuresKHR(
		commandBuffer,
		1,
		&accelerationBuildGeometryInfo,
		accelerationBuildStructureRangeInfos.data());
	vulkanDevice->flushCommandBuffer(commandBuffer, queue);

	deleteScratchBuffer(scratchBuffer);
}

/*
	The top level acceleration structure contains the scene's object instances
*/
void VulkanExample::createTopLevelAccelerationStructure()
{
	VkTransformMatrixKHR transformMatrix = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f };

	VkAccelerationStructureInstanceKHR instance{};
	instance.transform = transformMatrix;
	instance.instanceCustomIndex = 0;
	instance.mask = 0xFF;
	instance.instanceShaderBindingTableRecordOffset = 0;
	instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
	instance.accelerationStructureReference = bottomLevelAS.deviceAddress;

	// Buffer for instance data
	vks::Buffer instancesBuffer;
	VK_CHECK_RESULT(vulkanDevice->createBuffer(
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&instancesBuffer,
		sizeof(VkAccelerationStructureInstanceKHR),
		&instance));

	VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
	instanceDataDeviceAddress.deviceAddress = getBufferDeviceAddress(instancesBuffer.buffer);

	VkAccelerationStructureGeometryKHR accelerationStructureGeometry = vks::initializers::accelerationStructureGeometryKHR();
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
	accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

	// Get size info
	VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = vks::initializers::accelerationStructureBuildGeometryInfoKHR();
	accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationStructureBuildGeometryInfo.geometryCount = 1;
	accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

	uint32_t primitive_count = 1;

	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = vks::initializers::accelerationStructureBuildSizesInfoKHR();
	vkGetAccelerationStructureBuildSizesKHR(
		device,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&accelerationStructureBuildGeometryInfo,
		&primitive_count,
		&accelerationStructureBuildSizesInfo);

	createAccelerationStructure(topLevelAS, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, accelerationStructureBuildSizesInfo);

	// Create a small scratch buffer used during build of the top level acceleration structure
	ScratchBuffer scratchBuffer = createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

	VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo = vks::initializers::accelerationStructureBuildGeometryInfoKHR();
	accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	accelerationBuildGeometryInfo.dstAccelerationStructure = topLevelAS.handle;
	accelerationBuildGeometryInfo.geometryCount = 1;
	accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
	accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
	accelerationStructureBuildRangeInfo.primitiveCount = 1;
	accelerationStructureBuildRangeInfo.primitiveOffset = 0;
	accelerationStructureBuildRangeInfo.firstVertex = 0;
	accelerationStructureBuildRangeInfo.transformOffset = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

	// Build the acceleration structure on the device via a one-time command buffer submission
	// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
	VkCommandBuffer commandBuffer = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	vkCmdBuildAccelerationStructuresKHR(
		commandBuffer,
		1,
		&accelerationBuildGeometryInfo,
		accelerationBuildStructureRangeInfos.data());
	vulkanDevice->flushCommandBuffer(commandBuffer, queue);

	deleteScratchBuffer(scratchBuffer);
	instancesBuffer.destroy();
}

/*
	Create the Shader Binding Tables that binds the programs and top-level acceleration structure

	SBT Layout used in this sample:

		/-----------\
		| raygen    |
		|-----------|
		| miss      |
		|-----------|
		| hit       |
		\-----------/

*/
void VulkanExample::createShaderBindingTables() {
	const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
	const uint32_t handleSizeAligned = vks::tools::alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
	const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());
	const uint32_t sbtSize = groupCount * handleSizeAligned;

	std::vector<uint8_t> shaderHandleStorage(sbtSize);
	VK_CHECK_RESULT(vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()));

	createShaderBindingTable(shaderBindingTables.raygen, 1);
	createShaderBindingTable(shaderBindingTables.miss, 1);
	createShaderBindingTable(shaderBindingTables.hit, 1);

	// Copy handles
	memcpy(shaderBindingTables.raygen.mapped, shaderHandleStorage.data(), handleSize);
	memcpy(shaderBindingTables.miss.mapped, shaderHandleStorage.data() + handleSizeAligned, handleSize);
	memcpy(shaderBindingTables.hit.mapped, shaderHandleStorage.data() + handleSizeAligned * 2, handleSize);
}

/*
	Create our ray tracing pipeline
*/
void VulkanExample::createRayTracingPipeline()
{
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
		// Binding 0: Acceleration structure
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0),
		// Binding 1: Storage image
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1),
		// Binding 2: Uniform buffer
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, 2),
		// Binding 3: Vertex buffer 
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 3),
		// Binding 4: Index buffer
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 4),
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayout));

	VkPipelineLayoutCreateInfo pPipelineLayoutCI = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
	VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCI, nullptr, &pipelineLayout));

	/*
		Setup ray tracing shader groups
	*/
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	VkSpecializationMapEntry specializationMapEntry = vks::initializers::specializationMapEntry(0, 0, sizeof(uint32_t));
	uint32_t maxRecursion = 4;
	VkSpecializationInfo specializationInfo = vks::initializers::specializationInfo(1, &specializationMapEntry, sizeof(maxRecursion), &maxRecursion);

	// Ray generation group
	{
		shaderStages.push_back(loadShader(getShadersPath() + "raytracingVK/raygen.rgen.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR));
		// Pass recursion depth for reflections to ray generation shader via specialization constant
		shaderStages.back().pSpecializationInfo = &specializationInfo;
		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(shaderGroup);
	}

	// Miss group
	{
		shaderStages.push_back(loadShader(getShadersPath() + "raytracingVK/miss.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR));
		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(shaderGroup);
	}

	// Closest hit group (with any-hit shader for transparency)
	{
		shaderStages.push_back(loadShader(getShadersPath() + "raytracingVK/closesthit.rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR));
		uint32_t closestHitIndex = static_cast<uint32_t>(shaderStages.size()) - 1;
		
		shaderStages.push_back(loadShader(getShadersPath() + "raytracingVK/anyhit.rahit.spv", VK_SHADER_STAGE_ANY_HIT_BIT_KHR));
		uint32_t anyHitIndex = static_cast<uint32_t>(shaderStages.size()) - 1;
		
		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.closestHitShader = closestHitIndex;
		shaderGroup.anyHitShader = anyHitIndex;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(shaderGroup);
	}

	VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI = vks::initializers::rayTracingPipelineCreateInfoKHR();
	rayTracingPipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
	rayTracingPipelineCI.pStages = shaderStages.data();
	rayTracingPipelineCI.groupCount = static_cast<uint32_t>(shaderGroups.size());
	rayTracingPipelineCI.pGroups = shaderGroups.data();
	rayTracingPipelineCI.maxPipelineRayRecursionDepth = std::min(uint32_t(4), rayTracingPipelineProperties.maxRayRecursionDepth);
	rayTracingPipelineCI.layout = pipelineLayout;
	VK_CHECK_RESULT(vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCI, nullptr, &pipeline));
}

void VulkanExample::getEnabledFeatures()
{
	// Enable features required for ray tracing using feature chaining via pNext		
	enabledBufferDeviceAddresFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	enabledBufferDeviceAddresFeatures.bufferDeviceAddress = VK_TRUE;

	enabledRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	enabledRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
	enabledRayTracingPipelineFeatures.pNext = &enabledBufferDeviceAddresFeatures;

	enabledAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	enabledAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
	enabledAccelerationStructureFeatures.pNext = &enabledRayTracingPipelineFeatures;

	deviceCreatepNextChain = &enabledAccelerationStructureFeatures;
}

