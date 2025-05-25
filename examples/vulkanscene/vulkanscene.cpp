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
#include "keycodes.hpp"

void VulkanExample::logToStdout()
{
    // Get and print the GPU device address of the debug buffer
    uint64_t debugBufferAddress = GetBufferDeviceAddress(sboBuffers.debugBuffer.buffer);
    std::cout << "\n=======================================================\n";
    std::cout << "DEBUG BUFFER INFO:" << std::endl;
    std::cout << "  GPU Address: 0x" << std::hex << std::setw(16) << std::setfill('0') << debugBufferAddress << std::dec << std::endl;
    std::cout << "=======================================================\n" << std::endl;
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
    vkCmdFillBuffer(oneTimeSubmitCmdBuffer, sboBuffers.ssboData.buffer, 0, SBO_BUFFER_DATA_SIZE, 0xAAAAAAAA);
    vkCmdFillBuffer(oneTimeSubmitCmdBuffer, sboBuffers.debugBuffer.buffer, 0, SBO_BUFFER_DEBUG_SIZE, 0xAAAAAAAA);

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

void VulkanExample::setupDescriptorPool()
{
    // Example uses one ubo and one image sampler
    std::vector<VkDescriptorPoolSize> poolSizes =
    {
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 12),
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 12)
    };

    VkDescriptorPoolCreateInfo descriptorPoolInfo =
        vks::initializers::descriptorPoolCreateInfo(
            (uint32_t)poolSizes.size(),
            poolSizes.data(),
            10);  // 10 descriptor sets allocated out of this pool

    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
}

void VulkanExample::setupDescriptorSetLayout()
{
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
    {
        // Binding 0 : uniform buffer
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0),
        // Binding 1 : storage buffer
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            1)
    };

    VkDescriptorSetLayoutCreateInfo descriptorLayout =
        vks::initializers::descriptorSetLayoutCreateInfo(
            setLayoutBindings.data(),
            (uint32_t)setLayoutBindings.size());

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &graphicsPipelines.descriptorSetLayout));

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
        vks::initializers::pipelineLayoutCreateInfo(
            &graphicsPipelines.descriptorSetLayout,
            1);

    VkPushConstantRange pushConstantRange;
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(graphicsPushConstantData);

    pPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pPipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &graphicsPipelines.pipelineLayout));
}

void VulkanExample::setupDescriptorSet()
{
    VkDescriptorSetAllocateInfo allocInfo =
        vks::initializers::descriptorSetAllocateInfo(
            descriptorPool,
            &graphicsPipelines.descriptorSetLayout,
            1);

    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &graphicsPipelines.descriptorSet));

    std::vector<VkWriteDescriptorSet> writeDescriptorSets =
    {
        // Binding 0 : Vertex shader uniform buffer
        vks::initializers::writeDescriptorSet(
            graphicsPipelines.descriptorSet,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            0,
            &uniformData.uboMVPBuffer.descriptor),
        // Binding 1 : Storage buffer 
        vks::initializers::writeDescriptorSet(
            graphicsPipelines.descriptorSet,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            1,
            &sboBuffers.ssboData.descriptor)
    };

    vkUpdateDescriptorSets(device, (uint32_t)writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
}

// Prepare and initialize uniform buffer containing shader uniforms
void VulkanExample::prepareUniformBuffers()
{
    vulkanDevice->createBuffer(
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &uniformData.uboMVPBuffer,
        sizeof(uboVS));
    VK_CHECK_RESULT(uniformData.uboMVPBuffer.map());
    updateUniformBuffers();
}

void VulkanExample::updateUniformBuffers()
{
    uboVS.projection = camera.matrices.perspective;
    uboVS.view = camera.matrices.view;
    uboVS.model = glm::mat4(1.0f);
    uboVS.normal = glm::inverseTranspose(uboVS.view * uboVS.model);
    uboVS.lightPos = lightPos;
    memcpy(uniformData.uboMVPBuffer.mapped, &uboVS, sizeof(uboVS));
}

void VulkanExample::draw()
{
    // Handle auto-cycling between animation patterns
    if (autoCycle) {
        // Change animation every frame
        selectedComputePipeline = currentFrameCounter % 5;
        selectedGraphicsPipeline = currentFrameCounter % 5;
    }

    const bool scg = false;

    buildGraphicsCommandBuffers(1 << currentBuffer);
    buildTransferCommandBuffers(1 << currentBuffer);
    buildComputeCommandBuffers(1 << currentBuffer);

    // Get new value for currentBuffer and prepare the frame (this gets semaphores.presentComplete)
    VulkanExampleBase::prepareFrame();

    //-------------------------------------------------------------------------
    // 1. Transfer submit first - waits on presentComplete (or, waits on nothing on first frame) and signals graphics (and also compute if SCG is on)
    std::vector<VkCommandBuffer> transferCmdBuffersToSubmit;
    transferCmdBuffersToSubmit.push_back(copyCmdBuffers[currentBuffer]);

    transferSubmitInfo.commandBufferCount = (uint32_t)transferCmdBuffersToSubmit.size();
    transferSubmitInfo.pCommandBuffers = transferCmdBuffersToSubmit.data();

    transferSubmitInfo.waitSemaphoreCount = currentFrameCounter == 0 ? 0 : 1;
    transferSubmitInfo.pWaitSemaphores = &semaphores.presentComplete; // vkAcquireNextImage signals it
    VkPipelineStageFlags transferWaitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    transferSubmitInfo.pWaitDstStageMask = &transferWaitStage;

    VkSemaphore transferSignalSemaphores[] = { graphicsReady, computeReady };
    transferSubmitInfo.signalSemaphoreCount = scg ? 2 : 1;
    transferSubmitInfo.pSignalSemaphores = transferSignalSemaphores;

    VK_CHECK_RESULT(vkQueueSubmit(transferQueue, 1, &transferSubmitInfo, VK_NULL_HANDLE));


    //-------------------------------------------------------------------------
    // 2. Graphics submit - waits on graphicsReady and signals computeReady (or nothing if SCG is active)
    std::vector<VkCommandBuffer> graphicsCmdBuffersToSubmit;
    if (!currentFrameCounter) {
        // do only one time
        graphicsCmdBuffersToSubmit.push_back(oneTimeSubmitCmdBuffer);
    }
    graphicsCmdBuffersToSubmit.push_back(drawCmdBuffers[currentBuffer]);

    VkSubmitInfo graphicsSubmitInfo = {};
    graphicsSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    graphicsSubmitInfo.commandBufferCount = (uint32_t)graphicsCmdBuffersToSubmit.size();
    graphicsSubmitInfo.pCommandBuffers = graphicsCmdBuffersToSubmit.data();

    graphicsSubmitInfo.waitSemaphoreCount = 1;
    graphicsSubmitInfo.pWaitSemaphores = &graphicsReady;
    VkPipelineStageFlags graphicsWaitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    graphicsSubmitInfo.pWaitDstStageMask = &graphicsWaitStage;

    graphicsSubmitInfo.pSignalSemaphores = &computeReady;
    graphicsSubmitInfo.signalSemaphoreCount = scg ? 0 : 1;
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &graphicsSubmitInfo, VK_NULL_HANDLE));


    //-------------------------------------------------------------------------
    // 3. Compute submit - waits on computeReady and signals renderComplete
    std::vector<VkCommandBuffer> computeCmdBuffersToSubmit;
    computeCmdBuffersToSubmit.push_back(computeCmdBuffers[currentBuffer]);

    computeSubmitInfo.commandBufferCount = (uint32_t)computeCmdBuffersToSubmit.size();
    computeSubmitInfo.pCommandBuffers = computeCmdBuffersToSubmit.data();

    computeSubmitInfo.waitSemaphoreCount = 1;
    computeSubmitInfo.pWaitSemaphores = &computeReady;
    VkPipelineStageFlags computeWaitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    computeSubmitInfo.pWaitDstStageMask = &computeWaitStage;

    computeSubmitInfo.signalSemaphoreCount = 1;
    computeSubmitInfo.pSignalSemaphores = &semaphores.renderComplete;

    VK_CHECK_RESULT(vkQueueSubmit(computeQueue, 1, &computeSubmitInfo, VK_NULL_HANDLE));


    // Submit frame for presentation - queuePresent waits on renderComplete
    VulkanExampleBase::submitFrame();
}

VULKAN_EXAMPLE_MAIN()