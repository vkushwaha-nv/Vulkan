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
    return vkGetBufferDeviceAddress(device, &bufferDevice);
}

void VulkanExample::createBuffers()
{
    uint32_t sboUsageFlags = (VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    uint32_t vboUsageFlags = (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | sboUsageFlags);
    VkDeviceSize vertexDataSize = sizeof(Vertex) * NUM_MAX_VERTICES;

    VK_CHECK_RESULT(vulkanDevice->createBuffer(sboUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &sboBuffers.animatedVertexBuffer, vertexDataSize));
    VK_CHECK_RESULT(vulkanDevice->createBuffer(sboUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &sboBuffers.ssboData, SBO_BUFFER_DATA_SIZE));
    VK_CHECK_RESULT(vulkanDevice->createBuffer(sboUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &sboBuffers.debugBuffer, SBO_BUFFER_DEBUG_SIZE));

    VK_CHECK_RESULT(vulkanDevice->createBuffer(vboUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, vertexDataSize));
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
    VkDeviceSize vertexDataSize = sizeof(Vertex) * NUM_MAX_VERTICES;
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

    VK_CHECK_RESULT(vkBeginCommandBuffer(oneTimeSubmitCmdBuffer, &cmdBufInfo));

    // Fill other SSBOs buffer with random data
    vkCmdFillBuffer(oneTimeSubmitCmdBuffer, sboBuffers.ssboData.buffer, 0, SBO_BUFFER_DATA_SIZE, 0x33333333);
    vkCmdFillBuffer(oneTimeSubmitCmdBuffer, sboBuffers.debugBuffer.buffer, 0, SBO_BUFFER_DEBUG_SIZE, 0x44444444);

    // fill vertex buffer
    {
        std::vector<Vertex> vertexData;
        vertexData.resize(NUM_MAX_VERTICES);

        // A cube has 6 faces, each face has 2 triangles (6 vertices)
        const int verticesPerFace = 6;
        const int verticesPerCube = verticesPerFace * 6;  // 6 faces per cube
        const int gridSize = 24;  // 24x24x24 grid
        const int numCubes = gridSize * gridSize * gridSize;  // 13,824 cubes
        const float spacing = 1.2f;  // Space between cubes
        const float offset = -(gridSize * spacing) / 2.0f;  // Center the grid

        // Create multiple cubes with slight offsets
        for (int cube = 0; cube < numCubes; cube++) {
            // Calculate offset for this cube
            float offsetX = (cube % gridSize) * spacing + offset;
            float offsetY = ((cube / gridSize) % gridSize) * spacing + offset;
            float offsetZ = (cube / (gridSize * gridSize)) * spacing + offset;

            // Front face (red)
            for (int i = 0; i < verticesPerFace; i++) {
                float x = (i == 0 || i == 1 || i == 4) ? -0.5f : 0.5f;
                float y = (i == 0 || i == 3 || i == 4) ? -0.5f : 0.5f;
                float z = 0.5f;
                int idx = cube * verticesPerCube + i;
                vertexData[idx].position[0] = x + offsetX;
                vertexData[idx].position[1] = y + offsetY;
                vertexData[idx].position[2] = z + offsetZ;
                vertexData[idx].position[3] = 1.0f;
                vertexData[idx].color[0] = 1.0f;  // Red
                vertexData[idx].color[1] = 0.0f;
                vertexData[idx].color[2] = 0.0f;
            }

            // Back face (green)
            for (int i = 0; i < verticesPerFace; i++) {
                float x = (i == 0 || i == 1 || i == 4) ? 0.5f : -0.5f;
                float y = (i == 0 || i == 3 || i == 4) ? -0.5f : 0.5f;
                float z = -0.5f;
                int idx = cube * verticesPerCube + verticesPerFace + i;
                vertexData[idx].position[0] = x + offsetX;
                vertexData[idx].position[1] = y + offsetY;
                vertexData[idx].position[2] = z + offsetZ;
                vertexData[idx].position[3] = 1.0f;
                vertexData[idx].color[0] = 0.0f;
                vertexData[idx].color[1] = 1.0f;  // Green
                vertexData[idx].color[2] = 0.0f;
            }

            // Top face (blue)
            for (int i = 0; i < verticesPerFace; i++) {
                float x = (i == 0 || i == 1 || i == 4) ? -0.5f : 0.5f;
                float y = 0.5f;
                float z = (i == 0 || i == 3 || i == 4) ? 0.5f : -0.5f;
                int idx = cube * verticesPerCube + verticesPerFace * 2 + i;
                vertexData[idx].position[0] = x + offsetX;
                vertexData[idx].position[1] = y + offsetY;
                vertexData[idx].position[2] = z + offsetZ;
                vertexData[idx].position[3] = 1.0f;
                vertexData[idx].color[0] = 0.0f;
                vertexData[idx].color[1] = 0.0f;
                vertexData[idx].color[2] = 1.0f;  // Blue
            }

            // Bottom face (yellow)
            for (int i = 0; i < verticesPerFace; i++) {
                float x = (i == 0 || i == 1 || i == 4) ? -0.5f : 0.5f;
                float y = -0.5f;
                float z = (i == 0 || i == 3 || i == 4) ? -0.5f : 0.5f;
                int idx = cube * verticesPerCube + verticesPerFace * 3 + i;
                vertexData[idx].position[0] = x + offsetX;
                vertexData[idx].position[1] = y + offsetY;
                vertexData[idx].position[2] = z + offsetZ;
                vertexData[idx].position[3] = 1.0f;
                vertexData[idx].color[0] = 1.0f;
                vertexData[idx].color[1] = 1.0f;  // Yellow
                vertexData[idx].color[2] = 0.0f;
            }

            // Right face (magenta)
            for (int i = 0; i < verticesPerFace; i++) {
                float x = 0.5f;
                float y = (i == 0 || i == 1 || i == 4) ? -0.5f : 0.5f;
                float z = (i == 0 || i == 3 || i == 4) ? 0.5f : -0.5f;
                int idx = cube * verticesPerCube + verticesPerFace * 4 + i;
                vertexData[idx].position[0] = x + offsetX;
                vertexData[idx].position[1] = y + offsetY;
                vertexData[idx].position[2] = z + offsetZ;
                vertexData[idx].position[3] = 1.0f;
                vertexData[idx].color[0] = 1.0f;
                vertexData[idx].color[1] = 0.0f;
                vertexData[idx].color[2] = 1.0f;  // Magenta
            }

            // Left face (cyan)
            for (int i = 0; i < verticesPerFace; i++) {
                float x = -0.5f;
                float y = (i == 0 || i == 1 || i == 4) ? -0.5f : 0.5f;
                float z = (i == 0 || i == 3 || i == 4) ? -0.5f : 0.5f;
                int idx = cube * verticesPerCube + verticesPerFace * 5 + i;
                vertexData[idx].position[0] = x + offsetX;
                vertexData[idx].position[1] = y + offsetY;
                vertexData[idx].position[2] = z + offsetZ;
                vertexData[idx].position[3] = 1.0f;
                vertexData[idx].color[0] = 0.0f;
                vertexData[idx].color[1] = 1.0f;
                vertexData[idx].color[2] = 1.0f;  // Cyan
            }
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
        
        // Insert a memory barrier to ensure vertex buffer updates are visible
        VkBufferMemoryBarrier bufferBarrier = vks::initializers::bufferMemoryBarrier();
        bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        bufferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufferBarrier.buffer = vertexBuffer.buffer;
        bufferBarrier.offset = 0;
        bufferBarrier.size = vertexDataSize;
        
        vkCmdPipelineBarrier(
            oneTimeSubmitCmdBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            1, &bufferBarrier,
            0, nullptr);
            
        // Copy the vertex buffer data to the animated vertex buffer
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = vertexDataSize;
        vkCmdCopyBuffer(oneTimeSubmitCmdBuffer, vertexBuffer.buffer, sboBuffers.animatedVertexBuffer.buffer, 1, &copyRegion);
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
    prepareGraphicsPipelines();
    setupDescriptorPool();
    createBuffers();

    setupDescriptorSet();

    //----------------VKKK--------------------------
    prepareCompute();
    createCommandPoolAndBuffers();

    // Create semaphores for synchronization
    VkSemaphoreCreateInfo semaphoreCreateInfo = vks::initializers::semaphoreCreateInfo();
    VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &graphicsReady));
    VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &computeReady));

    buildOneTimeSubmitCommandBuffers();

    buildGraphicsCommandBuffers(7); // build all 3 command buffers the first time
    buildTransferCommandBuffers(7); // build all 3 command buffers the first time
    buildComputeCommandBuffers(7);  // build all 3 command buffers the first time

    prepared = true;
}

void VulkanExample::draw()
{
    // Handle auto-cycling between animation patterns
    if (autoCycle) {
        // Change animation every frame
        selectedComputePipeline = currentFrameCounter % 5;
        selectedGraphicsPipeline = currentFrameCounter % 5;
    }
    
    buildGraphicsCommandBuffers(1 << currentBuffer);
    buildTransferCommandBuffers(1 << currentBuffer);
    buildComputeCommandBuffers(1 << currentBuffer);

    // Get new value for currentBuffer and prepare the frame (this gets semaphores.presentComplete)
    VulkanExampleBase::prepareFrame();

    // 1. Transfer submit first - waits on presentComplete and signals graphicsComputeReady
    std::vector<VkCommandBuffer> transferCmdBuffersToSubmit;
    transferCmdBuffersToSubmit.push_back(copyCmdBuffers[currentBuffer]);
    
    transferSubmitInfo.commandBufferCount = (uint32_t)transferCmdBuffersToSubmit.size();
    transferSubmitInfo.pCommandBuffers = transferCmdBuffersToSubmit.data();
    
    // Wait for presentation semaphore
    transferSubmitInfo.waitSemaphoreCount = 1;
    transferSubmitInfo.pWaitSemaphores = &semaphores.presentComplete;
    VkPipelineStageFlags transferWaitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    transferSubmitInfo.pWaitDstStageMask = &transferWaitStage;
    
    // Signal completion for graphics and compute to start (signal both semaphores)
    VkSemaphore transferSignalSemaphores[] = {graphicsReady, computeReady};
    transferSubmitInfo.signalSemaphoreCount = 2;
    transferSubmitInfo.pSignalSemaphores = transferSignalSemaphores;
    
    VK_CHECK_RESULT(vkQueueSubmit(transferQueue, 1, &transferSubmitInfo, VK_NULL_HANDLE));

    // 2. Compute submit - waits on computeReady and signals nothing
    std::vector<VkCommandBuffer> computeCmdBuffersToSubmit;
    computeCmdBuffersToSubmit.push_back(computeCmdBuffers[currentBuffer]);
    
    computeSubmitInfo.commandBufferCount = (uint32_t)computeCmdBuffersToSubmit.size();
    computeSubmitInfo.pCommandBuffers = computeCmdBuffersToSubmit.data();
    
    // Wait for transfer to complete
    computeSubmitInfo.waitSemaphoreCount = 1;
    computeSubmitInfo.pWaitSemaphores = &computeReady;
    VkPipelineStageFlags computeWaitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    computeSubmitInfo.pWaitDstStageMask = &computeWaitStage;
    
    // Compute doesn't signal any semaphores
    computeSubmitInfo.signalSemaphoreCount = 0;
    
    VK_CHECK_RESULT(vkQueueSubmit(computeQueue, 1, &computeSubmitInfo, VK_NULL_HANDLE));

    // 3. Graphics submit - waits on graphicsReady and signals renderComplete
    std::vector<VkCommandBuffer> graphicsCmdBuffersToSubmit;
    if (!currentFrameCounter) {
        // do only one time
        graphicsCmdBuffersToSubmit.push_back(oneTimeSubmitCmdBuffer);
    }
    graphicsCmdBuffersToSubmit.push_back(drawCmdBuffers[currentBuffer]);

    // Create graphics submit info
    VkSubmitInfo graphicsSubmitInfo = {};
    graphicsSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    graphicsSubmitInfo.commandBufferCount = (uint32_t)graphicsCmdBuffersToSubmit.size();
    graphicsSubmitInfo.pCommandBuffers = graphicsCmdBuffersToSubmit.data();
    
    // Wait for transfer to complete via graphicsReady semaphore
    graphicsSubmitInfo.waitSemaphoreCount = 1;
    graphicsSubmitInfo.pWaitSemaphores = &graphicsReady;
    VkPipelineStageFlags graphicsWaitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    graphicsSubmitInfo.pWaitDstStageMask = &graphicsWaitStage;
    
    // Signal renderComplete semaphore for the presentation engine
    graphicsSubmitInfo.signalSemaphoreCount = 1;
    graphicsSubmitInfo.pSignalSemaphores = &semaphores.renderComplete;
    
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &graphicsSubmitInfo, VK_NULL_HANDLE));

    // Submit frame for presentation
    VulkanExampleBase::submitFrame();
}
