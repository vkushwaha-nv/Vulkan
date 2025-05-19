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

VulkanExample::VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
{
    srand((unsigned int)time(NULL));
    title = "Vulkan Demo Scene - (c) by Sascha Willems";
    camera.type = Camera::CameraType::lookat;
    //camera.flipY = true;
    camera.setPosition(glm::vec3(0.0f, 0.0f, -40.0f));
    camera.setRotation(glm::vec3(15.0f, 0.0f, 0.0f));
    camera.setRotationSpeed(0.5f);
    camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 256.0f);

    //Enable device extensions
    enabledDeviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);

}

VulkanExample::~VulkanExample()
{
    //destroy compute pipeline stuff
    vkDestroyPipeline(device, computePipelines.pipeline1, nullptr);
    vkDestroyPipeline(device, computePipelines.pipeline2, nullptr);
    vkDestroyPipeline(device, computePipelines.pipeline3, nullptr);
    vkDestroyPipeline(device, computePipelines.pipeline4, nullptr);
    vkDestroyPipeline(device, computePipelines.pipeline5, nullptr);
    vkDestroyPipelineLayout(device, computePipelines.pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, computePipelines.descriptorSetLayout, nullptr);

    //destroy graphics pipeline
    vkDestroyPipeline(device, graphicsPipelines.pipeline1, nullptr);
    vkDestroyPipeline(device, graphicsPipelines.pipeline2, nullptr);
    vkDestroyPipeline(device, graphicsPipelines.pipeline3, nullptr);
    vkDestroyPipeline(device, graphicsPipelines.pipeline4, nullptr);
    vkDestroyPipeline(device, graphicsPipelines.pipeline5, nullptr);
    vkDestroyPipelineLayout(device, graphicsPipelines.pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, graphicsPipelines.descriptorSetLayout, nullptr);

    sboBuffers.ssboData.destroy();
    sboBuffers.debugBuffer.destroy();

    vertexBuffer.destroy();

    // Clean up semaphores
    vkDestroySemaphore(device, graphicsReady, nullptr);
    vkDestroySemaphore(device, computeReady, nullptr);

    destroyCommandBuffers();
    vkDestroyCommandPool(device, copyCommandPool, nullptr);
    vkDestroyCommandPool(device, graphicsCommandPool, nullptr);
    vkDestroyCommandPool(device, computeCommandPool, nullptr);
}

uint64_t VulkanExample::GetBufferDeviceAddress(VkBuffer buffer)
{
    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
    vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));

    VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
    bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAI.buffer = buffer;
    return vkGetBufferDeviceAddressKHR(device, &bufferDeviceAI);
}

void VulkanExample::loadAssets()
{
    // No textures to load
}

void VulkanExample::prepare()
{
    VulkanExampleBase::prepare();

    vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));

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

    logToStdout();
    prepared = true;
}

void VulkanExample::viewChanged()
{
    updateUniformBuffers();
}

// Enable physical device features required for this example
void VulkanExample::getEnabledFeatures()
{
    // Enable vertex pipeline stores and atomics for storage buffer operations in vertex shader
    if (deviceFeatures.vertexPipelineStoresAndAtomics) {
        enabledFeatures.vertexPipelineStoresAndAtomics = VK_TRUE;
    }

    // Enable fill mode non-solid for wireframe rendering
    if (deviceFeatures.fillModeNonSolid) {
        enabledFeatures.fillModeNonSolid = VK_TRUE;
    }

    // Enable anisotropic filtering if supported
    if (deviceFeatures.samplerAnisotropy) {
        enabledFeatures.samplerAnisotropy = VK_TRUE;
    }

    // Enable features
    enabledBufferDeviceAddresFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    enabledBufferDeviceAddresFeatures.bufferDeviceAddress = VK_TRUE;
    deviceCreatepNextChain = &enabledBufferDeviceAddresFeatures;
}

void VulkanExample::TriggerCrash(OperationType opType, CrashType crashType)
{
    switch (opType) {
        case OperationType::Graphics:
            graphicsCrashType = static_cast<uint32_t>(crashType);
            break;
        case OperationType::Compute:
            computeCrashType = static_cast<uint32_t>(crashType);
            break;
        case OperationType::Transfer:
            transferCrashType = static_cast<uint32_t>(crashType);
            break;
    }
}

void VulkanExample::keyPressed(uint32_t keyCode)
{
    /*
    * Project Overview:
    *
    * This VulkanScene demo showcases a complete Vulkan workflow with graphics, compute,
    * and transfer operations working in parallel across different queue families.
    *
    * There are a total of 5 graphics pipelines and 5 compute pipelines, that alternate
    * each frame doing the animation. Each pipeline uses a different shader that does
    * slightly different animation.
    *
    * Workflow per frame:
    * 1. Graphics Pipeline: Reads vertex positions and colors from buffer A (vertexBuffer),
    *    applies shader effects to color data based on the selected graphics pipeline (draw1-5.vert),
    *    and renders the scene with animated colors.
    *
    * 2. Compute Pipeline: Reads the same vertex data, but processes vertex positions
    *    using compute shaders (compute1-5.comp). The animated vertices are written
    *    to buffer B (animatedVertexBuffer).
    *
    * 3. Transfer Operation: Copies the processed data from animatedVertexBuffer back tovertexBuffer,
    *    completing the cycle for the next frame.
    *
    * Synchronization:
    * - Each operation runs on its dedicated queue (graphics/compute/transfer)
    * - Semaphores ensure proper execution order between operations
    * - The transfer operation waits for presentation to complete before starting
    * - Graphics and compute operations wait for the transfer to complete
    *
    * Crash Testing:
    * The demo includes deliberate crash mechanisms to test GPU error handling:
    * - Graphics crash: Buffer out-of-bounds, division by zero, or infinite loop in vertex shader (only draw1.vert can cause crash)
    * - Compute crash: Similar crash types in compute shader with more data-dependent patterns  (only compute1.comp can cause crash)
    * - Transfer crash: Attempts to access invalid memory with a large offset (invalid offset is passed in vkCmdCopyBuffer)

    // Use below key bindings:
    // SPACE: Reset everything to default (no crashes and toggle between pipelines per frame)
    // F1-F5: Switch between graphics & compute pipelines

    // 1: Access address 0 in graphics pipeline (draw1.vert only)
    // 2: Generate infinite loop crash in graphics pipeline (draw1.vert only)
    // 3: (Not reliable) Generate out-of-bounds crash in graphics pipeline (changes are in draw1.vert only)
    // 4: (Not reliable) Generate division by zero crash in graphics pipeline (draw1.vert only)

    // 5: Access address 0 in compute pipeline (compute1.comp only)
    // 6: Generate infinite loop crash in compute pipeline (compute1.comp only)
    // 7: (Not reliable) Generate out-of-bounds crash in compute pipeline (compute1.comp only)
    // 8: (Not reliable) Generate division by zero crash in compute pipeline (compute1.comp only)

    // 9: Generate crash in transfer operation
    */

    switch (keyCode) {
    case KEY_F1:
        selectedComputePipeline = 0; // compute1.comp - Wave animation
        selectedGraphicsPipeline = 0; // draw1 shader - Basic patterns
        autoCycle = false; // Manual selection disables auto-cycling
        break;
    case KEY_F2:
        selectedComputePipeline = 1; // compute2.comp - Spiral wave
        selectedGraphicsPipeline = 1; // draw2 shader - Grid pattern
        autoCycle = false; // Manual selection disables auto-cycling
        break;
    case KEY_F3:
        selectedComputePipeline = 2; // compute3.comp - Pulsating effect
        selectedGraphicsPipeline = 2; // draw3 shader - Spiral pattern
        autoCycle = false; // Manual selection disables auto-cycling
        break;
    case KEY_F4:
        selectedComputePipeline = 3; // compute4.comp - Twist animation 
        selectedGraphicsPipeline = 3; // draw4 shader - Horizontal bands
        autoCycle = false; // Manual selection disables auto-cycling
        break;
    case KEY_F5:
        selectedComputePipeline = 4; // compute5.comp - Breathing animation
        selectedGraphicsPipeline = 4; // draw5 shader - Neon rings
        autoCycle = false; // Manual selection disables auto-cycling
        break;

    // Crashes:
    case 0x31: TriggerCrash(OperationType::Graphics, CrashType::AccessAddressZero); break;
    case 0x32: TriggerCrash(OperationType::Graphics, CrashType::InfiniteLoop); break;
    case 0x33: TriggerCrash(OperationType::Graphics, CrashType::OutOfBounds); break;

    case 0x35: TriggerCrash(OperationType::Compute, CrashType::AccessAddressZero); break;
    case 0x36: TriggerCrash(OperationType::Compute, CrashType::InfiniteLoop); break;
    case 0x37: TriggerCrash(OperationType::Compute, CrashType::OutOfBounds); break;

    case 0x39: TriggerCrash(OperationType::Transfer, CrashType::OutOfBounds); break;

    case KEY_SPACE:
        // restore auto-cycling and reset crash state
        graphicsCrashType = static_cast<uint32_t>(CrashType::None);
        computeCrashType = static_cast<uint32_t>(CrashType::None);
        transferCrashType = static_cast<uint32_t>(CrashType::None);
        autoCycle = true;
        break;
    }
}

void VulkanExample::render()
{
    if (!prepared)
        return;
    if (MAX_DRAW_FRAMES == 0 || currentFrameCounter < MAX_DRAW_FRAMES) {
        draw();
    }
    if (MAX_DRAW_FRAMES != 0 && currentFrameCounter > MAX_DRAW_FRAMES) {
        exit(0);
    }
    currentFrameCounter++;

    //Crash based on frame counter
    //if (currentFrameCounter == 0x109) {
    //    TriggerCrash(OperationType::Graphics, CrashType::AccessAddressZero);
    //}
}
