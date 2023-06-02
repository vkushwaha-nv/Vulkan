/*
* Vulkan Demo Scene
*
* Don't take this a an example, it's more of a personal playground
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* Note : Different license than the other examples!
*
* This code is licensed under the Mozilla Public License Version 2.0 (http://opensource.org/licenses/MPL-2.0)
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"
#include "vulkanscene.h"

void VulkanExample::createBuffers()
{
    // Create 5 SBO buffers on device
    uint32_t usageFlags = (VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    VK_CHECK_RESULT(vulkanDevice->createBuffer(usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &sboBuffers.buffer1, SBO_BUFFER_MAX_SIZE));
    VK_CHECK_RESULT(vulkanDevice->createBuffer(usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &sboBuffers.buffer2, SBO_BUFFER_MAX_SIZE));
    VK_CHECK_RESULT(vulkanDevice->createBuffer(usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &sboBuffers.buffer3, SBO_BUFFER_MAX_SIZE));
    VK_CHECK_RESULT(vulkanDevice->createBuffer(usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &sboBuffers.buffer4, SBO_BUFFER_MAX_SIZE));
    VK_CHECK_RESULT(vulkanDevice->createBuffer(usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &sboBuffers.buffer5, SBO_BUFFER_MAX_SIZE));
}

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

void VulkanExample::createCommandPoolAndBuffers()
{
    // Create graphics cmdPool and graphics command buffers
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = vulkanDevice->queueFamilyIndices.graphics;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &graphicsCommandPool));

    graphicsCmdBuffers.resize(SWAP_CHAIN_IMAGE_COUNT);
    VkCommandBufferAllocateInfo cmdBufAllocateInfo =
        vks::initializers::commandBufferAllocateInfo(
            graphicsCommandPool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            static_cast<uint32_t>(graphicsCmdBuffers.size()));
    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, graphicsCmdBuffers.data()));

    cmdBufAllocateInfo.commandBufferCount = 1;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &oneTimeSubmitCmdBuffer));

    // Create copy cmdPool and copy command buffers
    VkCommandPoolCreateInfo copycmdPoolInfo = {};
    copycmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    copycmdPoolInfo.queueFamilyIndex = vulkanDevice->queueFamilyIndices.transfer;
    copycmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(device, &copycmdPoolInfo, nullptr, &copyCommandPool));

    copyCmdBuffers.resize(SWAP_CHAIN_IMAGE_COUNT);
    VkCommandBufferAllocateInfo cmdBufAllocateInfoCopy =
        vks::initializers::commandBufferAllocateInfo(
            copyCommandPool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            static_cast<uint32_t>(copyCmdBuffers.size()));
    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfoCopy, copyCmdBuffers.data()));

    // Create compute cmdPool and compute command buffers
    VkCommandPoolCreateInfo computecmdPoolInfo = {};
    computecmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    computecmdPoolInfo.queueFamilyIndex = vulkanDevice->queueFamilyIndices.compute;
    computecmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(device, &computecmdPoolInfo, nullptr, &computeCommandPool));

    computeCmdBuffers.resize(SWAP_CHAIN_IMAGE_COUNT);
    VkCommandBufferAllocateInfo cmdBufAllocateInfoCompute =
        vks::initializers::commandBufferAllocateInfo(
            computeCommandPool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            static_cast<uint32_t>(computeCmdBuffers.size()));
    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfoCompute, computeCmdBuffers.data()));
}

void VulkanExample::buildOneTimeSubmitCommandBuffers()
{
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();   

    VK_CHECK_RESULT(vkBeginCommandBuffer(oneTimeSubmitCmdBuffer, &cmdBufInfo));

    // Copy identifier
    int pData[] = { 0x657921, 0x1000 };
    vkCmdUpdateBuffer(oneTimeSubmitCmdBuffer, sboBuffers.buffer1.buffer, 0, sizeof(uint32_t) * 2, pData);

    // fill a buffer
    vkCmdFillBuffer(oneTimeSubmitCmdBuffer, sboBuffers.buffer1.buffer, 0, SBO_BUFFER_MAX_SIZE, 0x11111111);
    vkCmdFillBuffer(oneTimeSubmitCmdBuffer, sboBuffers.buffer2.buffer, 0, SBO_BUFFER_MAX_SIZE, 0x22222222);
    vkCmdFillBuffer(oneTimeSubmitCmdBuffer, sboBuffers.buffer3.buffer, 0, SBO_BUFFER_MAX_SIZE, 0x33333333);
    vkCmdFillBuffer(oneTimeSubmitCmdBuffer, sboBuffers.buffer4.buffer, 0, SBO_BUFFER_MAX_SIZE, 0x44444444);
    vkCmdFillBuffer(oneTimeSubmitCmdBuffer, sboBuffers.buffer5.buffer, 0, SBO_BUFFER_MAX_SIZE, 0x55555555);

    VK_CHECK_RESULT(vkEndCommandBuffer(oneTimeSubmitCmdBuffer));
}

void VulkanExample::addCopyCommands(VkCommandBuffer cmdBuffer, uint32_t copyCount, VkDeviceSize copySize)
{
    VkBufferCopy copyRegion = {};
    copyRegion.size = copySize;
    const uint32_t numBuffers = 5; // 5 buffers
    VkBuffer bufferList[numBuffers] = {
        sboBuffers.buffer1.buffer,
        sboBuffers.buffer2.buffer,
        sboBuffers.buffer3.buffer,
        sboBuffers.buffer4.buffer,
        sboBuffers.buffer5.buffer,
    };

    for (uint32_t i=0; i<copyCount; i++) {
        int srcRandBuffer = rand() % numBuffers;
        int dstRandBuffer = rand() % numBuffers;
        if (srcRandBuffer == dstRandBuffer) {
            dstRandBuffer++;
            dstRandBuffer = dstRandBuffer % numBuffers;
        }
        vkCmdCopyBuffer(cmdBuffer, bufferList[srcRandBuffer], bufferList[dstRandBuffer], 1, &copyRegion);
    }
}

void VulkanExample::addDispatch(VkCommandBuffer cmdBuffer, uint32_t size_x, uint32_t size_y, uint32_t size_z, uint32_t complexity)
{
    // pick a pipeline based on complexity
    {
        if (complexity >= NUM_COMPUTE_PIPELINES) {
            complexity = NUM_COMPUTE_PIPELINES - 1;
        }
        VkPipeline pipelineArray[] = {
            computePipelines.pipeline1,
            computePipelines.pipeline2,
            computePipelines.pipeline3,
            computePipelines.pipeline4,
            computePipelines.pipeline5
        };
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineArray[complexity]);
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

void VulkanExample::buildTransferCommandBuffers(uint32_t buildMask)
{
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
    for (int32_t i = 0; i < copyCmdBuffers.size(); ++i)
    {
        if (((1 << i) & buildMask) == 0) {
            continue;
        }

        VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmdBuffers[i], &cmdBufInfo));

        // Copy identifier
        int pData[] = { 0x657921, 0x1001 };
        vkCmdUpdateBuffer(copyCmdBuffers[i], sboBuffers.buffer1.buffer, 0, sizeof(uint32_t) * 2, pData);

        addCopyCommands(copyCmdBuffers[i], 4 /* num copies */, SBO_BUFFER_MAX_SIZE/(1024 * 1024));

        VK_CHECK_RESULT(vkEndCommandBuffer(copyCmdBuffers[i]));
    }
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

        // Copy identifier
        int pData[] = { 0x657921, 0x1002 };
        vkCmdUpdateBuffer(computeCmdBuffers[i], sboBuffers.buffer1.buffer, 0, sizeof(uint32_t) * 2, pData);

        addCopyCommands(computeCmdBuffers[i], 1 /* num copies */, SBO_BUFFER_MAX_SIZE/(1024 * 1024));

        addDispatch(computeCmdBuffers[i], 102400, 10, 10,     COMPLEXITY_LEVEL_1);
        addDispatch(computeCmdBuffers[i], 102400, 10, 10,     COMPLEXITY_LEVEL_2);
        addDispatch(computeCmdBuffers[i], 102400, 10, 10,     COMPLEXITY_LEVEL_3);
        addDispatch(computeCmdBuffers[i], 102400, 10, 10,     COMPLEXITY_LEVEL_4);
        addDispatch(computeCmdBuffers[i], 102400, 10, 10,     COMPLEXITY_LEVEL_5);

        VK_CHECK_RESULT(vkEndCommandBuffer(computeCmdBuffers[i]));
    }
}

void VulkanExample::destroyCommandBuffers()
{
    vkFreeCommandBuffers(device, copyCommandPool, static_cast<uint32_t>(copyCmdBuffers.size()), copyCmdBuffers.data());
    vkFreeCommandBuffers(device, graphicsCommandPool, 1, &oneTimeSubmitCmdBuffer);
    vkFreeCommandBuffers(device, graphicsCommandPool, static_cast<uint32_t>(graphicsCmdBuffers.size()), graphicsCmdBuffers.data());
    vkFreeCommandBuffers(device, computeCommandPool, static_cast<uint32_t>(computeCmdBuffers.size()), computeCmdBuffers.data());
}

void VulkanExample::prepare()
{
    VulkanExampleBase::prepare();

    loadAssets();
    prepareUniformBuffers();
    setupDescriptorSetLayout();
    preparePipelines();
    setupDescriptorPool();
    setupDescriptorSet();
    buildDefaultCommandBuffers();

    //----------------VKKK--------------------------
    createBuffers();
    prepareCompute();
    createCommandPoolAndBuffers();

    buildOneTimeSubmitCommandBuffers();
    buildTransferCommandBuffers(7); // build all 3 command buffers the first time
    buildComputeCommandBuffers(7); // build all 3 command buffers the first time

    prepared = true;
}

void VulkanExample::draw()
{
    buildTransferCommandBuffers(1 << currentBuffer);
    
    buildComputeCommandBuffers(1 << currentBuffer);

    // get new value for currentBuffer
    VulkanExampleBase::prepareFrame();

    // graphics submit
    std::vector<VkCommandBuffer> graphicsCmdBuffersToSubmit;
    if (!currentFrameCounter) {
        // do only one time
        graphicsCmdBuffersToSubmit.push_back(oneTimeSubmitCmdBuffer);
    }
    graphicsCmdBuffersToSubmit.push_back(drawCmdBuffers[currentBuffer]);

    submitInfo.commandBufferCount = (uint32_t)graphicsCmdBuffersToSubmit.size();
    submitInfo.pCommandBuffers = graphicsCmdBuffersToSubmit.data();
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

    // transfer submit
    std::vector<VkCommandBuffer> transferCmdBuffersToSubmit;
    transferCmdBuffersToSubmit.push_back(copyCmdBuffers[currentBuffer]);
    transferSubmitInfo.commandBufferCount = (uint32_t)transferCmdBuffersToSubmit.size();
    transferSubmitInfo.pCommandBuffers = transferCmdBuffersToSubmit.data();
    VK_CHECK_RESULT(vkQueueSubmit(transferQueue, 1, &transferSubmitInfo, VK_NULL_HANDLE));

    // compute submit
    std::vector<VkCommandBuffer> computeCmdBuffersToSubmit;
    computeCmdBuffersToSubmit.push_back(computeCmdBuffers[currentBuffer]);
    computeSubmitInfo.commandBufferCount = (uint32_t)computeCmdBuffersToSubmit.size();
    computeSubmitInfo.pCommandBuffers = computeCmdBuffersToSubmit.data();
    VK_CHECK_RESULT(vkQueueSubmit(computeQueue, 1, &computeSubmitInfo, VK_NULL_HANDLE));

    // this uses the value of currentBuffer
    VulkanExampleBase::submitFrame();
}