
#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"
#include "vulkanscene.h"

void VulkanExample::addCopyCommands(VkCommandBuffer cmdBuffer, uint32_t copyCount, VkDeviceSize copySize)
{
    bool crash = false;//currentFrameCounter == 100;

    VkBufferCopy copyRegion = {};
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
        int pData[] = { 0x657921, 0x11000000 + i + currentFrameCounter};
        vkCmdUpdateBuffer(cmdBuffer, sboBuffers.buffer1.buffer, 0, sizeof(uint32_t) * 2, pData);

        copyRegion.size = (crash && i == 5) ? 0x65790 : copySize;
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

        addCopyCommands(copyCmdBuffers[i], 10 /* num copies */, SBO_BUFFER_MAX_SIZE/(1024));

        VK_CHECK_RESULT(vkEndCommandBuffer(copyCmdBuffers[i]));
    }
}
