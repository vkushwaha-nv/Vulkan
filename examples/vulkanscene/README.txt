
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

// 1: Generate out-of-bounds crash in graphics pipeline (changes are in draw1.vert only)
// 2: Generate division by zero crash in graphics pipeline (draw1.vert only)
// 3: Generate infinite loop crash in graphics pipeline (draw1.vert only)
// 4: Generate out-of-bounds crash in compute pipeline (compute1.comp only)
// 5: Generate division by zero crash in compute pipeline (compute1.comp only)
// 6: Generate infinite loop crash in compute pipeline (compute1.comp only)
// 7: Generate crash in transfer operation
