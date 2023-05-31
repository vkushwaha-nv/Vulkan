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
    VK_CHECK_RESULT(vulkanDevice->createBuffer(usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &sboBuffers.buffer1, RANDOM_BUFFER_SIZE));
    VK_CHECK_RESULT(vulkanDevice->createBuffer(usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &sboBuffers.buffer2, RANDOM_BUFFER_SIZE));
    VK_CHECK_RESULT(vulkanDevice->createBuffer(usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &sboBuffers.buffer3, RANDOM_BUFFER_SIZE));
    VK_CHECK_RESULT(vulkanDevice->createBuffer(usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &sboBuffers.buffer4, RANDOM_BUFFER_SIZE));
    VK_CHECK_RESULT(vulkanDevice->createBuffer(usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &sboBuffers.buffer5, RANDOM_BUFFER_SIZE));
}

void VulkanExample::prepareCompute()
{
    // Bindings
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        // Binding 0: input SBO
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 0),
        // Binding 1: Output SBO
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1),
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

    std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets = {
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &sboBuffers.buffer1.descriptor),
        vks::initializers::writeDescriptorSet(computePipelines.descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, &sboBuffers.buffer2.descriptor)
    };
    vkUpdateDescriptorSets(device, (uint32_t)computeWriteDescriptorSets.size(), computeWriteDescriptorSets.data(), 0, NULL);

    // Create compute shader pipelines
    VkComputePipelineCreateInfo computePipelineCreateInfo = vks::initializers::computePipelineCreateInfo(computePipelines.pipelineLayout, 0);
    std::string fileName = getShadersPath() + "vulkanscene/compute1.spv";
    computePipelineCreateInfo.stage = loadShader(fileName, VK_SHADER_STAGE_COMPUTE_BIT);
    VK_CHECK_RESULT(vkCreateComputePipelines(device, pipelineCache, 1, &computePipelineCreateInfo, nullptr, &computePipelines.pipeline1));
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
    vkCmdFillBuffer(oneTimeSubmitCmdBuffer, sboBuffers.buffer1.buffer, 0, RANDOM_BUFFER_SIZE, 0x11111111);
    vkCmdFillBuffer(oneTimeSubmitCmdBuffer, sboBuffers.buffer2.buffer, 0, RANDOM_BUFFER_SIZE, 0x22222222);
    vkCmdFillBuffer(oneTimeSubmitCmdBuffer, sboBuffers.buffer3.buffer, 0, RANDOM_BUFFER_SIZE, 0x33333333);
    vkCmdFillBuffer(oneTimeSubmitCmdBuffer, sboBuffers.buffer4.buffer, 0, RANDOM_BUFFER_SIZE, 0x44444444);
    vkCmdFillBuffer(oneTimeSubmitCmdBuffer, sboBuffers.buffer5.buffer, 0, RANDOM_BUFFER_SIZE, 0x55555555);

    VK_CHECK_RESULT(vkEndCommandBuffer(oneTimeSubmitCmdBuffer));
}

void VulkanExample::addCopyCommands(uint32_t copyCount, VkCommandBuffer cmdBuffer, VkDeviceSize copySize)
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

        addCopyCommands(20 /* num copies */, copyCmdBuffers[i], RANDOM_BUFFER_SIZE);

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

        addCopyCommands(10 /* num copies */, computeCmdBuffers[i], RANDOM_BUFFER_SIZE);

        vkCmdBindPipeline(computeCmdBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, computePipelines.pipeline1);
        vkCmdBindDescriptorSets(computeCmdBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, computePipelines.pipelineLayout, 0, 1, &computePipelines.descriptorSet, 0, 0);

        computePushConstantData.srcOffset = 0;
        computePushConstantData.dstOffset = 0;
        computePushConstantData.size = 100;
        vkCmdPushConstants(computeCmdBuffers[i],  computePipelines.pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 
            sizeof(computePushConstantData), &computePushConstantData);

        vkCmdDispatch(computeCmdBuffers[i], 1, 1, 1);


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