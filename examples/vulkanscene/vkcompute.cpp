#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"
#include "vulkanscene.h"

void VulkanExample::prepareCompute()
{
    // Bindings
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        // Binding 0: input SBO
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 0),
        // Binding 1: Output SBO
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1)
    };

    VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &computePipelines.descriptorSetLayout));

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
            vks::initializers::pipelineLayoutCreateInfo(&computePipelines.descriptorSetLayout, 1);
    VkPushConstantRange pushConstantRange;
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(computePushConstantData);

    pPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pPipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &computePipelines.pipelineLayout));

    VkDescriptorSetAllocateInfo allocInfo =
        vks::initializers::descriptorSetAllocateInfo(descriptorPool, &computePipelines.descriptorSetLayout, 1);

    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &computePipelines.descriptorSet));

    // update descriptor set
    std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets = {
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &vertexBuffer.descriptor),
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, &sboBuffers.animatedVertexBuffer.descriptor)
    };
    vkUpdateDescriptorSets(device, (uint32_t)computeWriteDescriptorSets.size(), computeWriteDescriptorSets.data(), 0, NULL);

    // Create compute shader pipelines
    VkComputePipelineCreateInfo computePipelineCreateInfo = vks::initializers::computePipelineCreateInfo(computePipelines.pipelineLayout, 0);
    std::string fileName1 = getShadersPath() + "vulkanscene/compute1.spv";
    computePipelineCreateInfo.stage = loadShader(fileName1, VK_SHADER_STAGE_COMPUTE_BIT);
    VK_CHECK_RESULT(vkCreateComputePipelines(device, pipelineCache, 1, &computePipelineCreateInfo, nullptr, &computePipelines.pipeline1));

    std::string fileName2 = getShadersPath() + "vulkanscene/compute2.spv";
    computePipelineCreateInfo.stage = loadShader(fileName2, VK_SHADER_STAGE_COMPUTE_BIT);
    VK_CHECK_RESULT(vkCreateComputePipelines(device, pipelineCache, 1, &computePipelineCreateInfo, nullptr, &computePipelines.pipeline2));

    std::string fileName3 = getShadersPath() + "vulkanscene/compute3.spv";
    computePipelineCreateInfo.stage = loadShader(fileName3, VK_SHADER_STAGE_COMPUTE_BIT);
    VK_CHECK_RESULT(vkCreateComputePipelines(device, pipelineCache, 1, &computePipelineCreateInfo, nullptr, &computePipelines.pipeline3));

    std::string fileName4 = getShadersPath() + "vulkanscene/compute4.spv";
    computePipelineCreateInfo.stage = loadShader(fileName4, VK_SHADER_STAGE_COMPUTE_BIT);
    VK_CHECK_RESULT(vkCreateComputePipelines(device, pipelineCache, 1, &computePipelineCreateInfo, nullptr, &computePipelines.pipeline4));

    std::string fileName5 = getShadersPath() + "vulkanscene/compute5.spv";
    computePipelineCreateInfo.stage = loadShader(fileName5, VK_SHADER_STAGE_COMPUTE_BIT);
    VK_CHECK_RESULT(vkCreateComputePipelines(device, pipelineCache, 1, &computePipelineCreateInfo, nullptr, &computePipelines.pipeline5));
}

void VulkanExample::addDispatch(VkCommandBuffer cmdBuffer, uint32_t size_x, uint32_t size_y, uint32_t size_z)
{
    bool crash = false; //currentFrameCounter == 100;
    uint32_t vertexDataSize = sizeof(Vertex) * NUM_MAX_VERTICES;

    {
        VkPipeline pipelineArray[] = {
            computePipelines.pipeline1,
            computePipelines.pipeline2,
            computePipelines.pipeline3,
            computePipelines.pipeline4,
            computePipelines.pipeline5,
        };
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineArray[selectedComputePipeline]);
    }

    // Bind the single descriptor set
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelines.pipelineLayout, 0, 1, &computePipelines.descriptorSet, 0, 0);

    // set push constants
    {
        computePushConstantData.srcOffset = 0;
        computePushConstantData.dstOffset = 0;
        
        if (crash) {
            uint64_t bufferAddress = GetBufferDeviceAddress(sboBuffers.ssboData.buffer);
            computePushConstantData.srcOffset = bufferAddress >> 32;
            computePushConstantData.dstOffset = bufferAddress & 0xFFFFFFFF;
        }
        computePushConstantData.size = vertexDataSize;
        
        // Set wave animation parameters
        static float totalTime = 0.0f;
        totalTime += 0.015f; // Slower, more subtle animation
        
        computePushConstantData.time = totalTime;
        computePushConstantData.waveHeight = 0.008f; // Very subtle amplitude
        computePushConstantData.waveFreq = 0.4f;     // Lower frequency for gentler waves
        computePushConstantData.temp4 = 0.0f;        // Not used
        
        vkCmdPushConstants(
            cmdBuffer,
            computePipelines.pipelineLayout,
            VK_SHADER_STAGE_COMPUTE_BIT,
            0,
            sizeof(computePushConstantData),
            &computePushConstantData);
    }

    uint32_t launchSizeX = (vertexDataSize + 31) / 32;  // Round up to next multiple of 32
    vkCmdDispatch(cmdBuffer, launchSizeX, 1, 1);
}

void VulkanExample::buildComputeCommandBuffers(uint32_t buildMask)
{
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
    for (int32_t i = 0; i < computeCmdBuffers.size(); ++i)
    {
        if (((1 << i) & buildMask) == 0) {
            continue;
        }
        VK_CHECK_RESULT(vkBeginCommandBuffer(computeCmdBuffers[i], &cmdBufInfo));

        int pData[] = { 0x657921, 0 };

        //add some copies on compute queue
        //addCopyCommands(computeCmdBuffers[i], 1 /* num copies */, SBO_BUFFER_MAX_SIZE/(1024 * 1024));

        pData[1] = 0x22000000 + currentFrameCounter;
        vkCmdUpdateBuffer(computeCmdBuffers[i], sboBuffers.debugBuffer.buffer, 0, sizeof(uint32_t) * 2, pData);
        addDispatch(computeCmdBuffers[i], 10, 10, 10);

 
        VK_CHECK_RESULT(vkEndCommandBuffer(computeCmdBuffers[i]));
    }
}
