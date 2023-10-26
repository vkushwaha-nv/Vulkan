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

uint64_t VulkanExample::GetBufferDeviceAddress(VkBuffer buffer)
{
    PFN_vkGetBufferDeviceAddress vkGetBufferDeviceAddress;
    vkGetBufferDeviceAddress = reinterpret_cast<PFN_vkGetBufferDeviceAddress>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddress"));
    VkBufferDeviceAddressInfo bufferDevice{};
    bufferDevice.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDevice.buffer = buffer;
    return 0; //vkGetBufferDeviceAddress(device, &bufferDevice);
}

void VulkanExample::createBuffers()
{
    uint32_t sboUsageFlags = (VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    uint32_t vboUsageFlags = (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | sboUsageFlags);

    VK_CHECK_RESULT(vulkanDevice->createBuffer(sboUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &sboBuffers.buffer1, SBO_BUFFER_MAX_SIZE));
    VK_CHECK_RESULT(vulkanDevice->createBuffer(sboUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &sboBuffers.buffer2, SBO_BUFFER_MAX_SIZE));
    VK_CHECK_RESULT(vulkanDevice->createBuffer(sboUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &sboBuffers.buffer3, SBO_BUFFER_MAX_SIZE));
    VK_CHECK_RESULT(vulkanDevice->createBuffer(sboUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &sboBuffers.buffer4, SBO_BUFFER_MAX_SIZE));
    VK_CHECK_RESULT(vulkanDevice->createBuffer(sboUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &sboBuffers.buffer5, SBO_BUFFER_MAX_SIZE));

    VK_CHECK_RESULT(vulkanDevice->createBuffer(vboUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, sizeof(Vertex) * NUM_MAX_VERTICES));
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

    // fill vertex buffer
    {
        std::vector<Vertex> vertexData;
        vertexData.resize(NUM_MAX_VERTICES);
        for (size_t i = 0; i < NUM_MAX_VERTICES; i++) {
            int randValueX = rand() % 1024;
            int randValueY = rand() % 1024;
            int randValueZ = rand() % 1024;
            int randValueW = rand() % 1024;
            int randValueR = rand() % 1024;
            int randValueG = rand() % 1024;
            int randValueB = rand() % 1024;
            vertexData[i].position[0] = randValueX / 1024.0f;
            vertexData[i].position[1] = randValueY / 1024.0f;
            vertexData[i].position[2] = randValueZ / 1024.0f;
            vertexData[i].position[3] = randValueW / 1024.0f;
            vertexData[i].color[0] = randValueR / 1024.0f;
            vertexData[i].color[1] = randValueG / 1024.0f;
            vertexData[i].color[2] = randValueB / 1024.0f;
        }
        size_t chunkSize = NUM_MAX_VERTICES_FILL_SIZE * sizeof(Vertex);
        assert(chunkSize < 32768); // there is a limit on size in vkCmdUpdateBuffer
        size_t filledSize = 0;
        size_t numLoops = NUM_MAX_VERTICES / NUM_MAX_VERTICES_FILL_SIZE;
        int t = 0;
        while (numLoops > 0) {
            int offset = t * NUM_MAX_VERTICES_FILL_SIZE;
            vkCmdUpdateBuffer(oneTimeSubmitCmdBuffer, vertexBuffer.buffer, filledSize, chunkSize, &vertexData[offset]);
            filledSize += chunkSize;
            numLoops--;
            t++;
        }

    }
    VK_CHECK_RESULT(vkEndCommandBuffer(oneTimeSubmitCmdBuffer));
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

    //----------------VKKK--------------------------
    createBuffers();
    prepareCompute();
    createCommandPoolAndBuffers();

    buildOneTimeSubmitCommandBuffers();

    buildGraphicsCommandBuffers(7); // build all 3 command buffers the first time
    buildTransferCommandBuffers(7); // build all 3 command buffers the first time
    buildComputeCommandBuffers(7);  // build all 3 command buffers the first time

    prepared = true;
}

void VulkanExample::draw()
{
    buildGraphicsCommandBuffers(1 << currentBuffer);

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