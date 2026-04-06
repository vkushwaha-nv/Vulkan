/*
* Vulkan Example - Using VK_EXT_graphics_pipeline_library
*
* Copyright (C) 2022-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"
#include <thread>
#include <mutex>
#include <chrono>
#include <fstream>
#include <limits>

#define PIPELINE_COUNT 3000
#define DESTROY_THREAD_COUNT 16

class VulkanExample: public VulkanExampleBase
{
public:
	bool linkTimeOptimization = true;

	vkglTF::Model scene;

	struct UniformData {
		glm::mat4 projection;
		glm::mat4 modelView;
		glm::vec4 lightPos = glm::vec4(0.0f, -2.0f, 1.0f, 0.0f);
	} uniformData;
	std::array<vks::Buffer, maxConcurrentFrames> uniformBuffers;

	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
	std::array<VkDescriptorSet, maxConcurrentFrames> descriptorSets{};

	VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT graphicsPipelineLibraryFeatures{};

	struct PipelineLibrary {
		std::vector<VkPipeline> vertexInputInterface;
		std::vector<VkPipeline> preRasterizationShaders;
		std::vector<VkPipeline> fragmentOutputInterface;
		std::vector<VkPipeline> fragmentShaders;
	} pipelineLibrary;

	std::vector<VkPipeline> pipelines{};

	struct ShaderInfo {
		uint32_t* code;
		size_t size;
	};

	std::mutex mutex;
	std::vector<VkPipelineCache> allPipelineCaches;
	std::vector<VkShaderModule> allShaderModules;

	// Create a new pipeline cache for each pipeline, to isolate them during free
	VkPipelineCache newPipelineCache()
	{
		VkPipelineCacheCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VkPipelineCache cache = VK_NULL_HANDLE;
		VK_CHECK_RESULT(vkCreatePipelineCache(device, &ci, nullptr, &cache));
		allPipelineCaches.push_back(cache);
		return cache;
	}

	bool  newPipelineCreated = false;
	int   vertexInputPipelineIndex = 0;

	uint32_t splitX{ 2 };
	uint32_t splitY{ 2 };

	// Wall time for preparePipelineLibrary() + executable pipeline creation (ms); used for teardown summary on stdout
	int64_t pipelineCreateWallTimeMs{ 0 };

	std::vector<glm::vec3> colors{};
	float rotation{ 0.0f };

	VulkanExample() : VulkanExampleBase()
	{
		title = "Graphics pipeline library";
#if defined(_WIN32)
		// Without this, a WIN32-subsystem build has no console, so std::cout is invisible unless
		// you redirect (e.g. > out.txt). Skip setupConsole when stdout is already a file or pipe so
		// redirection keeps working; validation mode already called setupConsole from the base ctor.
		{
			HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
			DWORD fileType = FILE_TYPE_UNKNOWN;
			if (hOut != nullptr && hOut != INVALID_HANDLE_VALUE) {
				fileType = GetFileType(hOut);
			}
			const bool stdoutRedirected = (fileType == FILE_TYPE_DISK || fileType == FILE_TYPE_PIPE);
			if (!stdoutRedirected && GetConsoleWindow() == nullptr) {
				setupConsole(title);
			}
		}
#endif
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -2.0f));
		camera.setRotation(glm::vec3(-25.0f, 15.0f, 0.0f));
		camera.setRotationSpeed(0.5f);

		// Enable required extensions
		enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_EXT_GRAPHICS_PIPELINE_LIBRARY_EXTENSION_NAME);

		// Enable required extension features
		graphicsPipelineLibraryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT;
		graphicsPipelineLibraryFeatures.graphicsPipelineLibrary = VK_TRUE;
		deviceCreatepNextChain = &graphicsPipelineLibraryFeatures;
	}

	~VulkanExample()
	{
		if (device) {
			// Flatten all pipelines into a single list matching the order of allPipelineCaches
			// Cache order: vertexInput[0..N], preRasterization[0..N], fragmentOutput[0..N], fragmentShaders[0..N], executables[0..N]
			std::vector<VkPipeline> allPipelines;
			allPipelines.reserve(allPipelineCaches.size());
			for (auto& p : pipelineLibrary.vertexInputInterface) allPipelines.push_back(p);
			for (auto& p : pipelineLibrary.preRasterizationShaders) allPipelines.push_back(p);
			for (auto& p : pipelineLibrary.fragmentOutputInterface) allPipelines.push_back(p);
			for (auto& p : pipelineLibrary.fragmentShaders) allPipelines.push_back(p);
			for (auto& p : pipelines) allPipelines.push_back(p);

			// Each thread alternates: vkDestroyPipeline, vkDestroyPipelineCache, vkDestroyShaderModule, ...
			int total = (int)allPipelines.size();
			int totalModules = (int)allShaderModules.size();
			int perThread = (total + DESTROY_THREAD_COUNT - 1) / DESTROY_THREAD_COUNT;
			std::vector<std::thread> threads;

			auto destroyStart = std::chrono::steady_clock::now();

			for (int t = 0; t < DESTROY_THREAD_COUNT; t++) {
				int start = t * perThread;
				int end = std::min(start + perThread, total);
				if (start >= total) break;
				threads.emplace_back([this, &allPipelines, start, end, totalModules]() {
					for (int i = start; i < end; i++) {
						vkDestroyPipeline(device, allPipelines[i], nullptr);
						vkDestroyPipelineCache(device, allPipelineCaches[i], nullptr);
						if (i < totalModules) {
							//vktodo vkDestroyShaderModule(device, allShaderModules[i], nullptr);
						}
					}
				});
			}

			for (auto& t : threads) {
				t.join();
			}

			auto destroyEnd = std::chrono::steady_clock::now();
			auto destroyMs = std::chrono::duration_cast<std::chrono::milliseconds>(destroyEnd - destroyStart).count();
			std::cout << "=== Total destroy time: " << destroyMs << " ms (" << total << " pipelines, " << (int)allPipelineCaches.size() << " caches, " << totalModules << " shader modules, " << DESTROY_THREAD_COUNT << " threads) ===" << std::endl;
			std::cout << "=== Pipeline wall time: create " << pipelineCreateWallTimeMs << " ms, destroy " << destroyMs << " ms";
			if (pipelineCreateWallTimeMs > 0) {
				std::cout << ", total " << (pipelineCreateWallTimeMs + destroyMs) << " ms";
			}
			std::cout << " ===" << std::endl;

			{
				std::ofstream logFile("C:\\cs1\\pipelineTime.txt", std::ios::app);
				if (logFile.is_open()) {
					logFile << "Destroy time: " << destroyMs << " ms | "
						<< total << " pipelines, "
						<< (int)allPipelineCaches.size() << " caches, "
						<< totalModules << " shader modules, "
						<< DESTROY_THREAD_COUNT << " threads" << std::endl;
					logFile.flush();
				}
			}

#if defined(_WIN32)
			// Keep the auxiliary console open so destroy timings stay visible (skip if no console or stdout redirected).
			if (GetConsoleWindow() != nullptr) {
				std::cout << std::endl << "Press Enter to close the console..." << std::flush;
				std::cin.clear();
				std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
				std::cin.get();
			}
#endif

			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
			for (auto& buffer : uniformBuffers) {
				buffer.destroy();
			}
		}
	}

	void loadAssets()
	{
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
		scene.loadFromFile(getAssetPath() + "models/color_teapot_spheres.gltf", vulkanDevice, queue, glTFLoadingFlags);
	}

	void setupDescriptors()
	{
		// Pool
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxConcurrentFrames)
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, maxConcurrentFrames);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

		// Layout
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		// Sets per frame, just like the buffers themselves
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		for (auto i = 0; i < uniformBuffers.size(); i++) {
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i]));
			std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
				vks::initializers::writeDescriptorSet(descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].descriptor),
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
		};
	}

	// With VK_EXT_graphics_pipeline_library we don't need to create the shader module when loading it, but instead have the driver create it at linking time
	// So we use a custom function that only loads the required shader information without actually creating the shader module
	bool loadShaderFile(std::string fileName, ShaderInfo &shaderInfo)
	{
#if defined(__ANDROID__)
		// Load shader from compressed asset
		AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, fileName.c_str(), AASSET_MODE_STREAMING);
		assert(asset);
		size_t size = AAsset_getLength(asset);
		assert(size > 0);

		shaderInfo.size = size;
		shaderInfo.code = new uint32_t[size / 4];
		AAsset_read(asset, reinterpret_cast<char*>(shaderInfo.code), size);
		AAsset_close(asset);
		return true;
#else
		std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);

		if (is.is_open())
		{
			shaderInfo.size = is.tellg();
			is.seekg(0, std::ios::beg);
			shaderInfo.code = new uint32_t[shaderInfo.size];
			is.read(reinterpret_cast<char*>(shaderInfo.code), shaderInfo.size);
			is.close();
			return true;
		} else {
			std::cerr << "Error: Could not open shader file \"" << fileName << "\"" << "\n";
			throw std::runtime_error("Could open shader file");
			return false;
		}
#endif
	}

	// Create the shared pipeline parts up-front
	void preparePipelineLibrary()
	{
		// Shared layout
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		// Create a pipeline library for the vertex input interface
		// Each pipeline gets a unique combination of topology, primitive restart, stride, input rate, and attribute formats
		{
			VkGraphicsPipelineLibraryCreateInfoEXT libraryInfo{};
			libraryInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT;
			libraryInfo.flags = VK_GRAPHICS_PIPELINE_LIBRARY_VERTEX_INPUT_INTERFACE_BIT_EXT;

			// Available topologies to cycle through
			const VkPrimitiveTopology topologies[] = {
				VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
				VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
				VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
			};
			constexpr uint32_t topologyCount = 6;

			// Available vec3-compatible formats to cycle through for attributes
			const VkFormat vec3Formats[] = {
				VK_FORMAT_R32G32B32_SFLOAT,
				VK_FORMAT_R32G32B32_SINT,
				VK_FORMAT_R32G32B32_UINT,
			};
			constexpr uint32_t formatCount = 3;

			// Input rates to cycle through
			const VkVertexInputRate inputRates[] = {
				VK_VERTEX_INPUT_RATE_VERTEX,
				VK_VERTEX_INPUT_RATE_INSTANCE,
			};

			pipelineLibrary.vertexInputInterface.resize(PIPELINE_COUNT);
			for (int i = 0; i < PIPELINE_COUNT; i++) {
				// Vary primitive topology
				VkPrimitiveTopology topology = topologies[i % topologyCount];

				// Vary primitive restart enable (only meaningful for strip/fan topologies, but valid to set)
				VkBool32 primitiveRestart = ((i / topologyCount) % 2) ? VK_TRUE : VK_FALSE;

				VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
				inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				inputAssemblyState.topology = topology;
				inputAssemblyState.primitiveRestartEnable = primitiveRestart;

				// Vary input rate between vertex and instance
				VkVertexInputRate inputRate = inputRates[i % 2];

				// Vary stride by adding different padding amounts (base stride = 36 bytes for 3x vec3)
				uint32_t extraPadding = (i % 16) * 4;  // 0 to 60 bytes of extra padding
				uint32_t stride = sizeof(float) * 9 + extraPadding;

				VkVertexInputBindingDescription bindingDesc{};
				bindingDesc.binding = 0;
				bindingDesc.stride = stride;
				bindingDesc.inputRate = inputRate;

				// Vary attribute formats and offsets per pipeline
				// Rotate formats independently for each attribute to maximize combinations
				VkFormat posFormat   = vec3Formats[(i) % formatCount];
				VkFormat normFormat  = vec3Formats[(i / formatCount) % formatCount];
				VkFormat colorFormat = vec3Formats[(i / (formatCount * formatCount)) % formatCount];

				// Vary attribute offsets with small perturbations (multiples of 4 for alignment)
				uint32_t posOffset   = 0;
				uint32_t normOffset  = sizeof(float) * 3 + ((i % 4) * 4);
				uint32_t colorOffset = sizeof(float) * 6 + ((i % 8) * 4);

				VkVertexInputAttributeDescription attributes[3] = {};
				// Position
				attributes[0].location = 0;
				attributes[0].binding = 0;
				attributes[0].format = posFormat;
				attributes[0].offset = posOffset;
				// Normal
				attributes[1].location = 1;
				attributes[1].binding = 0;
				attributes[1].format = normFormat;
				attributes[1].offset = normOffset;
				// Color
				attributes[2].location = 2;
				attributes[2].binding = 0;
				attributes[2].format = colorFormat;
				attributes[2].offset = colorOffset;

				VkPipelineVertexInputStateCreateInfo vertexInputState{};
				vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				vertexInputState.vertexBindingDescriptionCount = 1;
				vertexInputState.pVertexBindingDescriptions = &bindingDesc;
				vertexInputState.vertexAttributeDescriptionCount = 3;
				vertexInputState.pVertexAttributeDescriptions = attributes;

				VkGraphicsPipelineCreateInfo pipelineLibraryCI{};
				pipelineLibraryCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
				pipelineLibraryCI.flags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT;
				pipelineLibraryCI.pNext = &libraryInfo;
				pipelineLibraryCI.pInputAssemblyState = &inputAssemblyState;
				pipelineLibraryCI.pVertexInputState = &vertexInputState;

				VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, newPipelineCache(), 1, &pipelineLibraryCI, nullptr, &pipelineLibrary.vertexInputInterface[i]));
			}
		}

		// Create pipeline libraries for the vertex shader stage — one per PIPELINE_COUNT with unique specialization constants
		{
			VkGraphicsPipelineLibraryCreateInfoEXT libraryInfo{};
			libraryInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT;
			libraryInfo.flags = VK_GRAPHICS_PIPELINE_LIBRARY_PRE_RASTERIZATION_SHADERS_BIT_EXT;

			VkDynamicState vertexDynamicStates[2] = {
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR };

			VkPipelineDynamicStateCreateInfo dynamicInfo{};
			dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicInfo.dynamicStateCount = 2;
			dynamicInfo.pDynamicStates = vertexDynamicStates;

			VkPipelineViewportStateCreateInfo viewportState = {};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.scissorCount = 1;

			VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);

			ShaderInfo shaderInfo{};
			loadShaderFile(getShadersPath() + "graphicspipelinelibrary/shared.vert.spv", shaderInfo);

			VkShaderModuleCreateInfo shaderModuleCI{};
			shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shaderModuleCI.codeSize = shaderInfo.size;
			shaderModuleCI.pCode = shaderInfo.code;

			VkPipelineShaderStageCreateInfo shaderStageCI{};
			shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCI.pNext = &shaderModuleCI;
			shaderStageCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
			shaderStageCI.pName = "main";

			VkGraphicsPipelineCreateInfo pipelineLibraryCI{};
			pipelineLibraryCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineLibraryCI.pNext = &libraryInfo;
			pipelineLibraryCI.renderPass = renderPass;
			pipelineLibraryCI.flags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT;
			pipelineLibraryCI.stageCount = 1;
			pipelineLibraryCI.pStages = &shaderStageCI;
			pipelineLibraryCI.layout = pipelineLayout;
			pipelineLibraryCI.pDynamicState = &dynamicInfo;
			pipelineLibraryCI.pViewportState = &viewportState;
			pipelineLibraryCI.pRasterizationState = &rasterizationState;

			pipelineLibrary.preRasterizationShaders.resize(PIPELINE_COUNT);
			for (int i = 0; i < PIPELINE_COUNT; i++) {
				// Each pipeline gets a unique VARIANT_ID specialization constant
				int32_t variantId = i;

				VkSpecializationMapEntry specMapEntry{};
				specMapEntry.constantID = 0;
				specMapEntry.offset = 0;
				specMapEntry.size = sizeof(int32_t);

				VkSpecializationInfo specInfo{};
				specInfo.mapEntryCount = 1;
				specInfo.pMapEntries = &specMapEntry;
				specInfo.dataSize = sizeof(int32_t);
				specInfo.pData = &variantId;

				shaderStageCI.pSpecializationInfo = &specInfo;

				// Also create a standalone shader module (unused, but allocated for testing free)
				VkShaderModule vertShaderModule = VK_NULL_HANDLE;
				VK_CHECK_RESULT(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &vertShaderModule));
				allShaderModules.push_back(vertShaderModule);

				VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, newPipelineCache(), 1, &pipelineLibraryCI, nullptr, &pipelineLibrary.preRasterizationShaders[i]));
			}

			delete[] shaderInfo.code;
		}

		// Create pipeline libraries for the fragment output interface — one per PIPELINE_COUNT with varied blend state
		{
			VkGraphicsPipelineLibraryCreateInfoEXT libraryInfo{};
			libraryInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT;
			libraryInfo.flags = VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_OUTPUT_INTERFACE_BIT_EXT;

			VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

			// Blend factors and ops to cycle through
			const VkBlendFactor srcColorFactors[] = {
				VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_SRC_COLOR,
				VK_BLEND_FACTOR_DST_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA };
			const VkBlendFactor dstColorFactors[] = {
				VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_FACTOR_ONE,
				VK_BLEND_FACTOR_DST_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR };
			const VkBlendOp blendOps[] = {
				VK_BLEND_OP_ADD, VK_BLEND_OP_SUBTRACT, VK_BLEND_OP_REVERSE_SUBTRACT,
				VK_BLEND_OP_MIN, VK_BLEND_OP_MAX };

			VkGraphicsPipelineCreateInfo pipelineLibraryCI{};
			pipelineLibraryCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineLibraryCI.pNext = &libraryInfo;
			pipelineLibraryCI.layout = pipelineLayout;
			pipelineLibraryCI.renderPass = renderPass;
			pipelineLibraryCI.flags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT;
			pipelineLibraryCI.pMultisampleState = &multisampleState;

			pipelineLibrary.fragmentOutputInterface.resize(PIPELINE_COUNT);
			for (int i = 0; i < PIPELINE_COUNT; i++) {
				// Vary blend enable, blend factors, blend ops, and color write mask per pipeline
				VkPipelineColorBlendAttachmentState blendAttachment{};
				blendAttachment.blendEnable = (i % 2) ? VK_TRUE : VK_FALSE;
				blendAttachment.srcColorBlendFactor = srcColorFactors[i % 5];
				blendAttachment.dstColorBlendFactor = dstColorFactors[i % 5];
				blendAttachment.colorBlendOp = blendOps[i % 5];
				blendAttachment.srcAlphaBlendFactor = srcColorFactors[(i + 1) % 5];
				blendAttachment.dstAlphaBlendFactor = dstColorFactors[(i + 2) % 5];
				blendAttachment.alphaBlendOp = blendOps[(i + 1) % 5];
				// Vary color write mask — cycle through different channel combinations
				const VkColorComponentFlags writeMasks[] = {
					0xf,  // RGBA
					VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT,  // RGB
					VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_A_BIT,  // RA
					VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT,  // GB
					VK_COLOR_COMPONENT_R_BIT,  // R only
					VK_COLOR_COMPONENT_G_BIT,  // G only
					VK_COLOR_COMPONENT_B_BIT,  // B only
				};
				blendAttachment.colorWriteMask = writeMasks[i % 7];

				VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachment);
				// Vary blend constants
				colorBlendState.blendConstants[0] = (float)(i % 10) / 10.0f;
				colorBlendState.blendConstants[1] = (float)((i + 3) % 10) / 10.0f;
				colorBlendState.blendConstants[2] = (float)((i + 6) % 10) / 10.0f;
				colorBlendState.blendConstants[3] = (float)((i + 9) % 10) / 10.0f;

				pipelineLibraryCI.pColorBlendState = &colorBlendState;
				VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, newPipelineCache(), 1, &pipelineLibraryCI, nullptr, &pipelineLibrary.fragmentOutputInterface[i]));
			}
		}
	}

	// Thread function kept for future use (e.g. deleting pipelines)
	void threadFn()
	{
		auto start = std::chrono::steady_clock::now();

		prepareNewPipeline();

		auto delta = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start);
		std::cout << "Pipeline created in " << delta.count() << " microseconds\n";
	}

	// Create a new pipeline using the pipeline library and a customized fragment shader
	// Used from a thread
	void prepareNewPipeline()
	{
		// Create the fragment shader part of the pipeline library with some random options
		VkGraphicsPipelineLibraryCreateInfoEXT libraryInfo{};
		libraryInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT;
		libraryInfo.flags = VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_SHADER_BIT_EXT;

		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineMultisampleStateCreateInfo  multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

		// Using the pipeline library extension, we can skip the pipeline shader module creation and directly pass the shader code to the pipeline
		ShaderInfo shaderInfo{};
		loadShaderFile(getShadersPath() + "graphicspipelinelibrary/uber.frag.spv", shaderInfo);

		VkShaderModuleCreateInfo shaderModuleCI{};
		shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleCI.codeSize = shaderInfo.size;
		shaderModuleCI.pCode = shaderInfo.code;

		VkPipelineShaderStageCreateInfo shaderStageCI{};
		shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCI.pNext = &shaderModuleCI;
		shaderStageCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStageCI.pName = "main";

		// Select lighting model and variant ID using specialization constants
		srand(benchmark.active ? 0 : ((unsigned int)time(NULL)));
		struct FragSpecData {
			int32_t lightingModel;
			int32_t variantId;
		} fragSpecData;
		fragSpecData.lightingModel = (int)(rand() % 4);
		fragSpecData.variantId = vertexInputPipelineIndex;  // sequential ID per pipeline

		// Two specialization map entries: constant_id=0 (LIGHTING_MODEL), constant_id=1 (VARIANT_ID)
		VkSpecializationMapEntry specializationMapEntries[2] = {};
		specializationMapEntries[0].constantID = 0;
		specializationMapEntries[0].offset = offsetof(FragSpecData, lightingModel);
		specializationMapEntries[0].size = sizeof(int32_t);
		specializationMapEntries[1].constantID = 1;
		specializationMapEntries[1].offset = offsetof(FragSpecData, variantId);
		specializationMapEntries[1].size = sizeof(int32_t);

		VkSpecializationInfo specializationInfo{};
		specializationInfo.mapEntryCount = 2;
		specializationInfo.pMapEntries = specializationMapEntries;
		specializationInfo.dataSize = sizeof(FragSpecData);
		specializationInfo.pData = &fragSpecData;

		shaderStageCI.pSpecializationInfo = &specializationInfo;

		// Also create a standalone shader module (unused, but allocated for testing free)
		VkShaderModule fragShaderModule = VK_NULL_HANDLE;
		VK_CHECK_RESULT(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &fragShaderModule));
		allShaderModules.push_back(fragShaderModule);

		VkGraphicsPipelineCreateInfo pipelineCI{};
		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCI.pNext = &libraryInfo;
		pipelineCI.flags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT;
		pipelineCI.stageCount = 1;
		pipelineCI.pStages = &shaderStageCI;
		pipelineCI.layout = pipelineLayout;
		pipelineCI.renderPass = renderPass;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pMultisampleState = &multisampleState;
		VkPipeline fragmentShader = VK_NULL_HANDLE;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, newPipelineCache(), 1, &pipelineCI, nullptr, &fragmentShader));

		// Create the pipeline using the pre-built pipeline library parts
		// Except for above fragment shader part all parts have been pre-built and will be re-used
		// Use the nth vertex input pipeline, cycling through the array
		int idx = vertexInputPipelineIndex % PIPELINE_COUNT;
		vertexInputPipelineIndex++;
		std::vector<VkPipeline> libraries = {
			pipelineLibrary.vertexInputInterface[idx],
			pipelineLibrary.preRasterizationShaders[idx],
			fragmentShader,
			pipelineLibrary.fragmentOutputInterface[idx] };

		// Link the library parts into a graphics pipeline
		VkPipelineLibraryCreateInfoKHR pipelineLibraryCI{};
		pipelineLibraryCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;
		pipelineLibraryCI.libraryCount = static_cast<uint32_t>(libraries.size());
		pipelineLibraryCI.pLibraries = libraries.data();

		// If set to true, we pass VK_PIPELINE_CREATE_LINK_TIME_OPTIMIZATION_BIT_EXT which will let the implementation do additional optimizations at link time
		// This trades in pipeline creation time for run-time performance
		bool optimized = true;

		VkGraphicsPipelineCreateInfo executablePipelineCI{};
		executablePipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		executablePipelineCI.pNext = &pipelineLibraryCI;
		executablePipelineCI.layout = pipelineLayout;
		if (linkTimeOptimization)
		{
			// If link time optimization is activated in the UI, we set the VK_PIPELINE_CREATE_LINK_TIME_OPTIMIZATION_BIT_EXT flag which will let the implementation do additional optimizations at link time
			// This trades in pipeline creation time for run-time performance
			executablePipelineCI.flags = VK_PIPELINE_CREATE_LINK_TIME_OPTIMIZATION_BIT_EXT;
		}

		VkPipeline executable = VK_NULL_HANDLE;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, newPipelineCache(), 1, &executablePipelineCI, nullptr, &executable));

		pipelines.push_back(executable);
		// Push fragment shader to list for deletion in the sample's destructor
		pipelineLibrary.fragmentShaders.push_back(fragmentShader);

		delete[] shaderInfo.code;
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		for (auto& buffer : uniformBuffers) {
			VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer, sizeof(UniformData), &uniformData));
			VK_CHECK_RESULT(buffer.map());
		}
	}

	void updateUniformBuffers()
	{
		if (!paused) {
			rotation += frameTimer * 0.1f;
		}
		camera.setPerspective(45.0f, ((float)width / (float)splitX) / ((float)height / (float)splitY), 0.1f, 256.0f);
		uniformData.projection = camera.matrices.perspective;
		uniformData.modelView = camera.matrices.view * glm::rotate(glm::mat4(1.0f), glm::radians(rotation * 360.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		memcpy(uniformBuffers[currentBuffer].mapped, &uniformData, sizeof(UniformData));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadAssets();
		prepareUniformBuffers();
		setupDescriptors();
		auto createStart = std::chrono::steady_clock::now();

		preparePipelineLibrary();

		// Pre-create all PIPELINE_COUNT executable pipelines up front
		for (int i = 0; i < PIPELINE_COUNT; i++) {
			prepareNewPipeline();
		}

		auto createEnd = std::chrono::steady_clock::now();
		auto createMs = std::chrono::duration_cast<std::chrono::milliseconds>(createEnd - createStart).count();
		pipelineCreateWallTimeMs = createMs;
		std::cout << "=== Total pipeline creation time: " << createMs << " ms (GPL libraries + " << PIPELINE_COUNT << " executable pipelines) ===" << std::endl;

		{
			std::ofstream logFile("C:\\cs1\\pipelineTime.txt", std::ios::app);
			if (logFile.is_open()) {
				logFile << "Create time: " << createMs << " ms | "
					<< PIPELINE_COUNT << " pipelines" << std::endl;
				logFile.flush();
			}
		}

		// Set up the viewport grid to show all pipelines at once
		splitX = (uint32_t)ceil(sqrt((double)PIPELINE_COUNT));
		splitY = splitX;

		prepared = true;
	}

	void buildCommandBuffer()
	{
		VkCommandBuffer cmdBuffer = drawCmdBuffers[currentBuffer];
		
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2]{};
		clearValues[0].color = defaultClearColor;
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.framebuffer = frameBuffers[currentImageIndex];

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentBuffer], 0, nullptr);
		scene.bindBuffers(cmdBuffer);

		// Render a viewport for each pipeline
		float w = (float)width / (float)splitX;
		float h = (float)height / (float)splitY;
		uint32_t idx = 0;
		for (uint32_t y = 0; y < splitX; y++) {
			for (uint32_t x = 0; x < splitY; x++) {
				VkViewport viewport{};
				viewport.x = w * (float)x;
				viewport.y = h * (float)y;
				viewport.width = w;
				viewport.height = h;
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;
				vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

				VkRect2D scissor{};
				scissor.extent.width = (uint32_t)w;
				scissor.extent.height = (uint32_t)h;
				scissor.offset.x = (uint32_t)w * x;
				scissor.offset.y = (uint32_t)h * y;
				vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

				if (pipelines.size() > idx) {
					vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[idx]);
					scene.draw(cmdBuffer);
				}

				idx++;
			}
		}

		drawUI(cmdBuffer);

		vkCmdEndRenderPass(cmdBuffer);

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
	}


	virtual void render()
	{
		if (!prepared)
			return;
		VulkanExampleBase::prepareFrame();
		updateUniformBuffers();
		buildCommandBuffer();
		VulkanExampleBase::submitFrame();
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		overlay->checkBox("Link time optimization", &linkTimeOptimization);
	}
};

VULKAN_EXAMPLE_MAIN()