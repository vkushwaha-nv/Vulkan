#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"
#include "vulkanscene.h"

void VulkanExample::prepareGraphicsPipelines()
{
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_LINE, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
    VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
    VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
    VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
    std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables, 0);
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
    VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(graphicsPipelines.pipelineLayout, renderPass, 0);
    pipelineCI.pInputAssemblyState = &inputAssemblyState;
    pipelineCI.pRasterizationState = &rasterizationState;
    pipelineCI.pColorBlendState = &colorBlendState;
    pipelineCI.pMultisampleState = &multisampleState;
    pipelineCI.pViewportState = &viewportState;
    pipelineCI.pDepthStencilState = &depthStencilState;
    pipelineCI.pDynamicState = &dynamicState;
    pipelineCI.stageCount = (uint32_t)shaderStages.size();
    pipelineCI.pStages = shaderStages.data();
    pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::UV, vkglTF::VertexComponent::Color });;

    shaderStages[0] = loadShader(getShadersPath() + "vulkanscene/draw1.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader(getShadersPath() + "vulkanscene/draw1.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    VkVertexInputBindingDescription vertexInputBinding = {};
    vertexInputBinding.binding = 0;
    vertexInputBinding.stride = sizeof(Vertex);
    vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributs;
    //	layout (location = 0) in vec4 inPos;
    //	layout (location = 1) in vec3 inColor;
    // Attribute location 0: Position
    vertexInputAttributs[0].binding = 0;
    vertexInputAttributs[0].location = 0;
    // Position attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
    vertexInputAttributs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertexInputAttributs[0].offset = offsetof(Vertex, position);
    // Attribute location 1: Color
    vertexInputAttributs[1].binding = 0;
    vertexInputAttributs[1].location = 1;
    // Color attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
    vertexInputAttributs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributs[1].offset = offsetof(Vertex, color);

    VkPipelineVertexInputStateCreateInfo vertexInputState = {};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = 1;
    vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;
    vertexInputState.vertexAttributeDescriptionCount = 2;
    vertexInputState.pVertexAttributeDescriptions = vertexInputAttributs.data();

    pipelineCI.pVertexInputState = &vertexInputState;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &graphicsPipelines.pipeline1));

    shaderStages[0] = loadShader(getShadersPath() + "vulkanscene/draw2.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader(getShadersPath() + "vulkanscene/draw2.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &graphicsPipelines.pipeline2));

    shaderStages[0] = loadShader(getShadersPath() + "vulkanscene/draw3.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader(getShadersPath() + "vulkanscene/draw3.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &graphicsPipelines.pipeline3));

    shaderStages[0] = loadShader(getShadersPath() + "vulkanscene/draw4.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader(getShadersPath() + "vulkanscene/draw4.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &graphicsPipelines.pipeline4));

    shaderStages[0] = loadShader(getShadersPath() + "vulkanscene/draw5.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader(getShadersPath() + "vulkanscene/draw5.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &graphicsPipelines.pipeline5));
}

void VulkanExample::addDraw(VkCommandBuffer cmdBuffer)
{
    bool crash = false; //currentFrameCounter == 100;

    VkPipeline pipelineList[] = {
        graphicsPipelines.pipeline1,
        graphicsPipelines.pipeline2,
        graphicsPipelines.pipeline3,
        graphicsPipelines.pipeline4,
        graphicsPipelines.pipeline5,
    };
    int randPipeline = 0; // rand() % 1; // vkkk force same pipeline
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, crash ? graphicsPipelines.pipeline5 : pipelineList[randPipeline]);
    VkDeviceSize vertexOffset = 0;
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer.buffer, &vertexOffset);

    if (crash) {
        graphicsPushConstantData.addressHi = 0xFED000;
        graphicsPushConstantData.addressLo = 0xFED000;
        vkCmdPushConstants(cmdBuffer, graphicsPipelines.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
            sizeof(graphicsPushConstantData), &graphicsPushConstantData);
    }

    vkCmdDraw(cmdBuffer, NUM_MAX_VERTICES - 1024, 1, 0, 0);
}

void VulkanExample::buildGraphicsCommandBuffers(uint32_t buildMask)
{
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

    VkClearValue clearValues[2];
    clearValues[0].color = defaultClearColor;
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = width;
    renderPassBeginInfo.renderArea.extent.height = height;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;

    for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
    {
        if (((1 << i) & buildMask) == 0) {
            continue;
        }
        renderPassBeginInfo.framebuffer = frameBuffers[i];

        VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

        // Copy identifier
        int pData[] = { 0x657921, 0x300001 };
        //vkCmdUpdateBuffer(drawCmdBuffers[i], sboBuffers.debugBuffer.buffer, 0, sizeof(uint32_t) * 2, pData);

        vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
        vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

        VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
        vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

        vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines.pipelineLayout, 0, 1, &graphicsPipelines.descriptorSet, 0, NULL);

        // add draws
        {
            pData[1] = 0x330000 + currentFrameCounter;
            //vkCmdUpdateBuffer(drawCmdBuffers[i], sboBuffers.debugBuffer.buffer, 0, sizeof(uint32_t) * 2, pData);
            addDraw(drawCmdBuffers[i]);
        }

        vkCmdEndRenderPass(drawCmdBuffers[i]);

        VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
    }
}
