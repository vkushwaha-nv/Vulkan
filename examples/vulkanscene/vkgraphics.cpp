#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"
#include "vulkanscene.h"

void VulkanExample::addDraw(VkCommandBuffer cmdBuffer, bool changeVtxOffsetEveryDraw)
{
    bool crash = false; //currentFrameCounter == 100;

    VkDeviceSize offset = changeVtxOffsetEveryDraw ? (rand() % 1024) * sizeof(Vertex) : 0;
    VkPipeline pipelineList[] = {
        graphicsPipeline1,
        graphicsPipeline2,
        graphicsPipeline3,
        graphicsPipeline4,
        graphicsPipeline5,
    };
    int randPipeline = rand() % 4;
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, crash ? graphicsPipeline5 : pipelineList[randPipeline]);
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer.buffer, &offset);

    if (crash) {
        graphicsPushConstantData.addressHi = 0xFED000;
        graphicsPushConstantData.addressLo = 0xFED000;
        vkCmdPushConstants(cmdBuffer,  pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
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
        vkCmdUpdateBuffer(drawCmdBuffers[i], sboBuffers.buffer1.buffer, 0, sizeof(uint32_t) * 2, pData);

        vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
        vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

        VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
        vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

        vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

        // add draws
        {
            pData[1] = 0x330000 + currentFrameCounter;
            vkCmdUpdateBuffer(drawCmdBuffers[i], sboBuffers.buffer1.buffer, 0, sizeof(uint32_t) * 2, pData);
            addDraw(drawCmdBuffers[i], false/*changeVtxOffsetEveryDraw*/);

        }

        vkCmdEndRenderPass(drawCmdBuffers[i]);

        VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
    }
}
