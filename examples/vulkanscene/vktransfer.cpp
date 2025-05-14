#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"
#include "vulkanscene.h"

void VulkanExample::addCopyCommands(VkCommandBuffer cmdBuffer, uint32_t copyCount, VkDeviceSize copySize)
{
    bool crash = false;//currentFrameCounter == 100;
    VkBufferCopy copyRegion = {};

    uint32_t pData[] = { 0x657921, 0x11000000 + currentFrameCounter};
    vkCmdUpdateBuffer(cmdBuffer, sboBuffers.ssboData.buffer, 0, sizeof(uint32_t) * 2, pData);

    copyRegion.size = copySize;
    //vkCmdCopyBuffer(cmdBuffer, sboBuffers.animatedVertexBuffer.buffer, vertexBuffer.buffer, 1, &copyRegion);
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
        vkCmdUpdateBuffer(copyCmdBuffers[i], sboBuffers.ssboData.buffer, 0, sizeof(uint32_t) * 2, pData);

        VkDeviceSize vertexDataSize = sizeof(Vertex) * NUM_MAX_VERTICES;
        addCopyCommands(copyCmdBuffers[i], 10 /* num copies */, vertexDataSize);

        VK_CHECK_RESULT(vkEndCommandBuffer(copyCmdBuffers[i]));
    }
}
