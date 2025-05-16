#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"
#include "vulkanscene.h"

void VulkanExample::addCopyCommands(VkCommandBuffer cmdBuffer, uint32_t copyCount, VkDeviceSize copySize)
{
    VkBufferCopy copyRegion = {};

    if (transferCrashType != 0) {
        // Create a crash by setting the destination offset to 3 GB
        copyRegion.dstOffset = 3ULL * 1024 * 1024 * 1024; // 3 GB offset
    }
    copyRegion.size = copySize;
    vkCmdCopyBuffer(cmdBuffer, sboBuffers.animatedVertexBuffer.buffer, vertexBuffer.buffer, 1, &copyRegion);
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
        int pData[] = { 0x657921, 0 };
        pData[1] = 0x11000000 | currentFrameCounter;
        
        // Calculate the debug offset for transfer queue (index 0)
        // Each entry takes sizeof(uint32_t) * 2 bytes, and we have 3 entries per frame
        uint32_t entrySize = sizeof(uint32_t);
        uint32_t frameEntrySize = entrySize * 3;
        uint32_t maxEntries = SBO_BUFFER_DEBUG_SIZE / frameEntrySize;
        uint32_t wrappedFrameIndex = currentFrameCounter % maxEntries;
        uint32_t debugOffset = (wrappedFrameIndex * 3 + 0) * entrySize;
        
        vkCmdUpdateBuffer(copyCmdBuffers[i], sboBuffers.debugBuffer.buffer, debugOffset, sizeof(uint32_t) * 2, pData);

        VkDeviceSize vertexDataSize = sizeof(Vertex) * NUM_MAX_VERTICES;
        addCopyCommands(copyCmdBuffers[i], 10 /* num copies */, vertexDataSize);

        VK_CHECK_RESULT(vkEndCommandBuffer(copyCmdBuffers[i]));
    }
}
