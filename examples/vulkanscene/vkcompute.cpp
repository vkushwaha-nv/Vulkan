
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
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1),
        // Binding 2: input image
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 2),
        // Binding 3: Output image
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 3),
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

    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &computePipelines.descriptorSet1));
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &computePipelines.descriptorSet2));
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &computePipelines.descriptorSet3));
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &computePipelines.descriptorSet4));
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &computePipelines.descriptorSet5));

    // update descriptor sets
    std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets1 = {
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &sboBuffers.buffer1.descriptor),
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, &sboBuffers.buffer2.descriptor),
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2, &textureList.tex1.descriptor),
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3, &textureList.tex2.descriptor)
    };
    vkUpdateDescriptorSets(device, (uint32_t)computeWriteDescriptorSets1.size(), computeWriteDescriptorSets1.data(), 0, NULL);
    std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets2 = {
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &sboBuffers.buffer2.descriptor),
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, &sboBuffers.buffer3.descriptor),
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2, &textureList.tex2.descriptor),
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3, &textureList.tex3.descriptor)
    };
    vkUpdateDescriptorSets(device, (uint32_t)computeWriteDescriptorSets2.size(), computeWriteDescriptorSets2.data(), 0, NULL);
    std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets3 = {
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &sboBuffers.buffer3.descriptor),
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, &sboBuffers.buffer4.descriptor),
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2, &textureList.tex3.descriptor),
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3, &textureList.tex4.descriptor)
    };
    vkUpdateDescriptorSets(device, (uint32_t)computeWriteDescriptorSets3.size(), computeWriteDescriptorSets3.data(), 0, NULL);
    std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets4 = {
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &sboBuffers.buffer4.descriptor),
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, &sboBuffers.buffer5.descriptor),
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2, &textureList.tex4.descriptor),
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3, &textureList.tex1.descriptor)
    };
    vkUpdateDescriptorSets(device, (uint32_t)computeWriteDescriptorSets4.size(), computeWriteDescriptorSets4.data(), 0, NULL);
    std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets5 = {
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &sboBuffers.buffer5.descriptor),
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, &sboBuffers.buffer1.descriptor),
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet5, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2, &textureList.tex3.descriptor),
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet5, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3, &textureList.tex1.descriptor)
    };
    vkUpdateDescriptorSets(device, (uint32_t)computeWriteDescriptorSets5.size(), computeWriteDescriptorSets5.data(), 0, NULL);

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

void VulkanExample::addDispatch(VkCommandBuffer cmdBuffer, uint32_t size_x, uint32_t size_y, uint32_t size_z, uint32_t complexity)
{
    bool crash = false; //currentFrameCounter == 100;

    // pick a pipeline based on complexity
    {
        if (complexity >= NUM_COMPUTE_PIPELINES) {
            //complexity is in range [0,3]
            complexity = NUM_COMPUTE_PIPELINES - 2;
        }
        VkPipeline pipelineArray[] = {
            computePipelines.pipeline1,
            computePipelines.pipeline2,
            computePipelines.pipeline3,
            computePipelines.pipeline4,
        };
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, crash ? computePipelines.pipeline5 : pipelineArray[complexity]);
    }

    // pick a random descriptor set
    {
        int randDescriptorSet = rand() % NUM_DESCRIPTOR_SETS;
        VkDescriptorSet descSetArray[] = {
            computePipelines.descriptorSet1,
            computePipelines.descriptorSet2,
            computePipelines.descriptorSet3,
            computePipelines.descriptorSet4,
            computePipelines.descriptorSet5
        };
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelines.pipelineLayout, 0, 1, &descSetArray[randDescriptorSet], 0, 0);
    }

    // set push constant
    {
        computePushConstantData.srcOffset = 0;
        computePushConstantData.dstOffset = 0;
        if (crash) {
            uint64_t bufferAddress = GetBufferDeviceAddress(sboBuffers.buffer3.buffer);
            computePushConstantData.srcOffset = bufferAddress >> 32;
            computePushConstantData.dstOffset = bufferAddress & 0xFFFFFFFF;
        }
        computePushConstantData.size = SBO_BUFFER_MAX_SIZE;
        computePushConstantData.temp1 = float(rand()%100 / 100.0f);
        computePushConstantData.temp2 = float(rand()%100 / 100.0f);
        computePushConstantData.temp3 = float(rand()%100 / 100.0f);
        computePushConstantData.temp4 = float(rand()%100 / 100.0f);
        vkCmdPushConstants(cmdBuffer,  computePipelines.pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
            sizeof(computePushConstantData), &computePushConstantData);
    }

    // don't let workgroup size be too big
    if (size_x > vulkanDevice->properties.limits.maxComputeWorkGroupCount[0]) {
        size_x = vulkanDevice->properties.limits.maxComputeWorkGroupCount[0];
    }
    if (size_y > vulkanDevice->properties.limits.maxComputeWorkGroupCount[1]) {
        size_y = vulkanDevice->properties.limits.maxComputeWorkGroupCount[1];
    }
    if (size_z > vulkanDevice->properties.limits.maxComputeWorkGroupCount[2]) {
        size_z = vulkanDevice->properties.limits.maxComputeWorkGroupCount[2];
    }
    vkCmdDispatch(cmdBuffer, size_x, size_y, size_z);
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
        vkCmdUpdateBuffer(computeCmdBuffers[i], sboBuffers.buffer1.buffer, 0, sizeof(uint32_t) * 2, pData);
        addDispatch(computeCmdBuffers[i], 10, 10, 10,     COMPLEXITY_LEVEL_1);

 
        VK_CHECK_RESULT(vkEndCommandBuffer(computeCmdBuffers[i]));
    }
}
