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

void VulkanExample::loadAssets()
{
    // No textures to load
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
}

void VulkanExample::viewChanged()
{
    updateUniformBuffers();
}

void VulkanExample::keyPressed(uint32_t keyCode)
{
    // F1-F5: Switch between graphics & compute pipelines
    // 1: Generate out-of-bounds crash in graphics pipeline (draw1.vert only)
    // 2: Generate division by zero crash in graphics pipeline (draw1.vert only)
    // 3: Generate infinite loop crash in graphics pipeline (draw1.vert only)

    // 4: Generate out-of-bounds crash in compute pipeline (compute1.comp only)
    // 5: Generate division by zero crash in compute pipeline (compute1.comp only)
    // 6: Generate infinite loop crash in compute pipeline (compute1.comp only)
    // 7: Generate crash in transfer operation

    // SPACE: Reset everything to default (no crashes and toggle between pipelines per frame)
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

    // Graphics crashes:
    case 0x31: //1
        graphicsCrashType = 1;
        break;

    case 0x32: //2
        graphicsCrashType = 2;
        break;

    case 0x33: //3
        graphicsCrashType = 3;
        break;

    // Compute crashes:
    case 0x34:
        computeCrashType = 1;
        break;

    case 0x35:
        computeCrashType = 2;
        break;

    case 0x36:
        computeCrashType = 3;
        break;

    // Transfer crash:
    case 0x37: //7
        transferCrashType = 1;
        break;

    case KEY_SPACE:
        // restore auto-cycling and reset crash state
        graphicsCrashType = 0;
        computeCrashType = 0;
        transferCrashType = 0;
        graphicsCrashValue1 = 0;
        graphicsCrashValue2 = 0;
        computeCrashValue1 = 0;
        computeCrashValue2 = 0;
        autoCycle = true;
        break;
    }
}

VULKAN_EXAMPLE_MAIN()