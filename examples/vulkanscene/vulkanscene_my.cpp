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

void VulkanExample::createSBOBuffers()
{
    // Create 5 SBO buffers on device
    uint32_t usageFlags = (VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    vulkanDevice->createBuffer(usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &randomBuffers.buffer1, RANDOM_BUFFER_SIZE);
    vulkanDevice->createBuffer(usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &randomBuffers.buffer2, RANDOM_BUFFER_SIZE);
    vulkanDevice->createBuffer(usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &randomBuffers.buffer3, RANDOM_BUFFER_SIZE);
    vulkanDevice->createBuffer(usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &randomBuffers.buffer4, RANDOM_BUFFER_SIZE);
    vulkanDevice->createBuffer(usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &randomBuffers.buffer5, RANDOM_BUFFER_SIZE);
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
    int pData[] = { 0x657921, 0x657922 };
    vkCmdUpdateBuffer(oneTimeSubmitCmdBuffer, randomBuffers.buffer1.buffer, 0, sizeof(uint32_t) * 2, pData);

    // fill a buffer
    vkCmdFillBuffer(oneTimeSubmitCmdBuffer, randomBuffers.buffer1.buffer, 0, RANDOM_BUFFER_SIZE, 0x11111111);
    vkCmdFillBuffer(oneTimeSubmitCmdBuffer, randomBuffers.buffer2.buffer, 0, RANDOM_BUFFER_SIZE, 0x22222222);
    vkCmdFillBuffer(oneTimeSubmitCmdBuffer, randomBuffers.buffer3.buffer, 0, RANDOM_BUFFER_SIZE, 0x33333333);
    vkCmdFillBuffer(oneTimeSubmitCmdBuffer, randomBuffers.buffer4.buffer, 0, RANDOM_BUFFER_SIZE, 0x44444444);
    vkCmdFillBuffer(oneTimeSubmitCmdBuffer, randomBuffers.buffer5.buffer, 0, RANDOM_BUFFER_SIZE, 0x55555555);

    VK_CHECK_RESULT(vkEndCommandBuffer(oneTimeSubmitCmdBuffer));
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
        int pData[] = { 0x657921, 0x657922 };
        vkCmdUpdateBuffer(copyCmdBuffers[i], randomBuffers.buffer1.buffer, 0, sizeof(uint32_t) * 2, pData);

        VkBufferCopy copyRegion = {};
        copyRegion.size = RANDOM_BUFFER_SIZE/1;
        vkCmdCopyBuffer(copyCmdBuffers[i], randomBuffers.buffer5.buffer, randomBuffers.buffer2.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(copyCmdBuffers[i], randomBuffers.buffer2.buffer, randomBuffers.buffer3.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(copyCmdBuffers[i], randomBuffers.buffer2.buffer, randomBuffers.buffer4.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(copyCmdBuffers[i], randomBuffers.buffer4.buffer, randomBuffers.buffer5.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(copyCmdBuffers[i], randomBuffers.buffer5.buffer, randomBuffers.buffer2.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(copyCmdBuffers[i], randomBuffers.buffer3.buffer, randomBuffers.buffer4.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(copyCmdBuffers[i], randomBuffers.buffer2.buffer, randomBuffers.buffer5.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(copyCmdBuffers[i], randomBuffers.buffer3.buffer, randomBuffers.buffer2.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(copyCmdBuffers[i], randomBuffers.buffer4.buffer, randomBuffers.buffer3.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(copyCmdBuffers[i], randomBuffers.buffer5.buffer, randomBuffers.buffer2.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(copyCmdBuffers[i], randomBuffers.buffer2.buffer, randomBuffers.buffer3.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(copyCmdBuffers[i], randomBuffers.buffer2.buffer, randomBuffers.buffer4.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(copyCmdBuffers[i], randomBuffers.buffer4.buffer, randomBuffers.buffer5.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(copyCmdBuffers[i], randomBuffers.buffer5.buffer, randomBuffers.buffer2.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(copyCmdBuffers[i], randomBuffers.buffer3.buffer, randomBuffers.buffer4.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(copyCmdBuffers[i], randomBuffers.buffer1.buffer, randomBuffers.buffer5.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(copyCmdBuffers[i], randomBuffers.buffer4.buffer, randomBuffers.buffer2.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(copyCmdBuffers[i], randomBuffers.buffer5.buffer, randomBuffers.buffer3.buffer, 1, &copyRegion);

       
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
        int pData[] = { 0x657921, 0x657922 };
        vkCmdUpdateBuffer(computeCmdBuffers[i], randomBuffers.buffer1.buffer, 0, sizeof(uint32_t) * 2, pData);

        // fill a buffer
        vkCmdFillBuffer(computeCmdBuffers[i], randomBuffers.buffer1.buffer, 0, RANDOM_BUFFER_SIZE, pData[0]);
        
         VkBufferCopy copyRegion = {};
        copyRegion.size = RANDOM_BUFFER_SIZE/1;
        vkCmdCopyBuffer(computeCmdBuffers[i], randomBuffers.buffer5.buffer, randomBuffers.buffer2.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(computeCmdBuffers[i], randomBuffers.buffer2.buffer, randomBuffers.buffer3.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(computeCmdBuffers[i], randomBuffers.buffer2.buffer, randomBuffers.buffer4.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(computeCmdBuffers[i], randomBuffers.buffer4.buffer, randomBuffers.buffer5.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(computeCmdBuffers[i], randomBuffers.buffer5.buffer, randomBuffers.buffer2.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(computeCmdBuffers[i], randomBuffers.buffer3.buffer, randomBuffers.buffer4.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(computeCmdBuffers[i], randomBuffers.buffer2.buffer, randomBuffers.buffer5.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(computeCmdBuffers[i], randomBuffers.buffer3.buffer, randomBuffers.buffer2.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(computeCmdBuffers[i], randomBuffers.buffer4.buffer, randomBuffers.buffer3.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(computeCmdBuffers[i], randomBuffers.buffer5.buffer, randomBuffers.buffer2.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(computeCmdBuffers[i], randomBuffers.buffer2.buffer, randomBuffers.buffer3.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(computeCmdBuffers[i], randomBuffers.buffer2.buffer, randomBuffers.buffer4.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(computeCmdBuffers[i], randomBuffers.buffer4.buffer, randomBuffers.buffer5.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(computeCmdBuffers[i], randomBuffers.buffer5.buffer, randomBuffers.buffer2.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(computeCmdBuffers[i], randomBuffers.buffer3.buffer, randomBuffers.buffer4.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(computeCmdBuffers[i], randomBuffers.buffer1.buffer, randomBuffers.buffer5.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(computeCmdBuffers[i], randomBuffers.buffer4.buffer, randomBuffers.buffer2.buffer, 1, &copyRegion);
        vkCmdCopyBuffer(computeCmdBuffers[i], randomBuffers.buffer5.buffer, randomBuffers.buffer3.buffer, 1, &copyRegion);


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
    createSBOBuffers();
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