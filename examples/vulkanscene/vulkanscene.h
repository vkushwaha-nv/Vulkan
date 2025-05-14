#define ENABLE_VALIDATION true

#define MAX_DRAW_FRAMES 0 // 0 = INF

#define SBO_BUFFER_DATA_SIZE (1 * 1024 * 1024ULL)
#define SBO_BUFFER_DEBUG_SIZE (32 * 1024 * 1024ULL)

#define NUM_MAX_VERTICES (512 * 1024ULL)
#define NUM_MAX_VERTICES_FILL_SIZE (1024ULL) // must be a multiple of NUM_MAX_VERTICES

#define SWAP_CHAIN_IMAGE_COUNT  3

#define NUM_DESCRIPTOR_SETS     5
#define NUM_COMPUTE_PIPELINES   5

class VulkanExample : public VulkanExampleBase
{
public:
    // Selected compute pipeline index (0-4 for pipeline1-pipeline5)
    int selectedComputePipeline = 0;
    
    // Selected graphics pipeline index (0-4 for pipeline1-pipeline5)
    int selectedGraphicsPipeline = 0;
    
    // Auto-cycling between pipelines
    bool autoCycle = true;
    
    // Controls whether to generate a crash
    uint32_t graphicsCrashType = 0;
    uint32_t computeCrashType = 0;
    uint32_t transferCrashType = 0;
    
    // Additional crash parameters
    uint32_t graphicsCrashValue1 = 0;
    uint32_t graphicsCrashValue2 = 0;
    uint32_t computeCrashValue1 = 0;
    uint32_t computeCrashValue2 = 0;
    
    // Synchronization primitives
    VkSemaphore graphicsReady = VK_NULL_HANDLE;
    VkSemaphore computeReady = VK_NULL_HANDLE;

    struct {
        vks::Buffer uboMVPBuffer;
    } uniformData;

    struct {
        glm::mat4 projection;
        glm::mat4 model;
        glm::mat4 normal;
        glm::mat4 view;
        glm::vec4 lightPos;
    } uboVS;

    struct {
        uint32_t srcOffset;
        uint32_t dstOffset;
        uint32_t size;
        float time;       // Animation time
        float waveHeight; // Wave height/amplitude
        float waveFreq;   // Wave frequency
        float temp4;      // Unused
        uint32_t crashType; // Type of crash to simulate (0 = none)
        uint32_t crashValue1; // Additional crash parameter 1
        uint32_t crashValue2; // Additional crash parameter 2
    } computePushConstantData;

    struct {
        float time;
        float animationTime;
        float colorMod;
        float colorShift;
        float pulseSpeed;
        float colorIntensity;
        uint32_t crashType; // Type of crash to simulate (0 = none)
        uint32_t crashValue1; // Additional crash parameter 1
        uint32_t crashValue2; // Additional crash parameter 2
    } graphicsPushConstantData;

    // Compute pipeline
    struct {
        VkPipelineLayout pipelineLayout;
        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet descriptorSet;
        VkPipeline pipeline1;
        VkPipeline pipeline2;
        VkPipeline pipeline3;
        VkPipeline pipeline4;
        VkPipeline pipeline5;
    } computePipelines;

    // Graphics pipeline
    struct {
        VkPipelineLayout pipelineLayout;
        VkDescriptorSet descriptorSet;
        VkDescriptorSetLayout descriptorSetLayout;
        VkPipeline pipeline1;
        VkPipeline pipeline2;
        VkPipeline pipeline3;
        VkPipeline pipeline4;
        VkPipeline pipeline5;
    } graphicsPipelines;

    glm::vec4 lightPos = glm::vec4(1.0f, 4.0f, 0.0f, 0.0f);

    struct Vertex {
        float position[4];
        float color[3];
    };
    vks::Buffer vertexBuffer;

    // 3 SBO buffers
    struct {
        vks::Buffer animatedVertexBuffer;
        vks::Buffer ssboData;
        vks::Buffer debugBuffer;
    } sboBuffers;

    // Graphics command pool
    VkCommandPool graphicsCommandPool;
    std::vector<VkCommandBuffer> graphicsCmdBuffers;
    VkCommandBuffer oneTimeSubmitCmdBuffer;

    // Copy command pool
    VkCommandPool copyCommandPool;
    std::vector<VkCommandBuffer> copyCmdBuffers;

    // compute command pool
    VkCommandPool computeCommandPool;
    std::vector<VkCommandBuffer> computeCmdBuffers;

    unsigned currentFrameCounter {0};
    // ------------------------------------------------------------------

    VulkanExample();
    ~VulkanExample();
    
    void loadAssets();
    void setupDescriptorPool();
    void setupDescriptorSetLayout();
    void setupDescriptorSet();
    void prepareCompute();
    void prepareGraphicsPipelines();
    
    // Prepare and initialize uniform buffer containing shader uniforms
    void prepareUniformBuffers(); 

    // ----VKKK------------------------------------------------------
    uint64_t GetBufferDeviceAddress(VkBuffer buffer);
    void createBuffers();
    void createCommandPoolAndBuffers();
    void buildOneTimeSubmitCommandBuffers();
    void buildGraphicsCommandBuffers(uint32_t buildMask);
    void buildTransferCommandBuffers(uint32_t buildMask);
    void addCopyCommands(VkCommandBuffer cmdBuffer, uint32_t copyCount, VkDeviceSize copySize);
    void addDraw(VkCommandBuffer cmdBuffer);
    void addDispatch(VkCommandBuffer cmdBuffer, uint32_t size_x, uint32_t size_y, uint32_t size_z);
    void buildComputeCommandBuffers(uint32_t buildMask);
    void destroyCommandBuffers();
    // ------------------------------------------------------------------


    void updateUniformBuffers();
    void draw(); 
    void prepare();
    virtual void render();
    virtual void viewChanged();
    virtual void keyPressed(uint32_t keyCode);
};

