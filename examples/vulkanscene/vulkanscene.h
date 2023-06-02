#define ENABLE_VALIDATION true

#define MAX_DRAW_FRAMES 0 // 0 = INF
#define SBO_BUFFER_MAX_SIZE (512 * 1024 * 1024ULL)

#define NUM_MAX_VERTICES (512 * 1024ULL)
#define NUM_MAX_VERTICES_FILL_SIZE (1024ULL) // must be a multiple of NUM_MAX_VERTICES

#define SWAP_CHAIN_IMAGE_COUNT  3

#define NUM_DESCRIPTOR_SETS     5
#define NUM_COMPUTE_PIPELINES   5

#define COMPLEXITY_LEVEL_1     0
#define COMPLEXITY_LEVEL_2     1
#define COMPLEXITY_LEVEL_3     2
#define COMPLEXITY_LEVEL_4     3
#define COMPLEXITY_LEVEL_5     4

static unsigned currentFrameCounter = 0;


class VulkanExample : public VulkanExampleBase
{
public:
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
        float temp1;
        float temp2;
        float temp3;
        float temp4;
    } computePushConstantData;

    // Compute pipeline
    struct {
        VkPipelineLayout pipelineLayout;
        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet descriptorSet1;
        VkDescriptorSet descriptorSet2;
        VkDescriptorSet descriptorSet3;
        VkDescriptorSet descriptorSet4;
        VkDescriptorSet descriptorSet5;
        VkPipeline pipeline1;
        VkPipeline pipeline2;
        VkPipeline pipeline3;
        VkPipeline pipeline4;
        VkPipeline pipeline5;
    } computePipelines;

    // Graphics pipeline
    VkPipelineLayout pipelineLayout;
    VkDescriptorSet descriptorSet;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipeline graphicsPipeline1;
    VkPipeline graphicsPipeline2;
    VkPipeline graphicsPipeline3;
    VkPipeline graphicsPipeline4;
    VkPipeline graphicsPipeline5;

    glm::vec4 lightPos = glm::vec4(1.0f, 4.0f, 0.0f, 0.0f);

    // 5 SBO buffers
    struct {
        vks::Buffer buffer1;
        vks::Buffer buffer2;
        vks::Buffer buffer3;
        vks::Buffer buffer4;
        vks::Buffer buffer5;
    } sboBuffers;

    struct {
        vks::Texture2D tex1;
        vks::Texture2D tex2;
        vks::Texture2D tex3;
        vks::Texture2D tex4;
    } textureList;


    struct Vertex {
	    float position[4];
	    float color[3];
    };
    vks::Buffer vertexBuffer;


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

    // ------------------------------------------------------------------

    VulkanExample();
    ~VulkanExample();
    
    void loadAssets();
    void setupDescriptorPool();
    void setupDescriptorSetLayout();
    void setupDescriptorSet();
    void prepareCompute();
    void preparePipelines();
    
    // Prepare and initialize uniform buffer containing shader uniforms
    void prepareUniformBuffers(); 

    // ----VKKK------------------------------------------------------
    void createBuffers();
    void createCommandPoolAndBuffers();
    void buildOneTimeSubmitCommandBuffers();
    void buildGraphicsCommandBuffers(uint32_t buildMask);
    void buildTransferCommandBuffers(uint32_t buildMask);
    void addCopyCommands(VkCommandBuffer cmdBuffer, uint32_t copyCount, VkDeviceSize copySize);
    void addDraw(VkCommandBuffer cmdBuffer, bool changeVtxOffsetEveryDraw);
    void addDispatch(VkCommandBuffer cmdBuffer, uint32_t size_x, uint32_t size_y, uint32_t size_z, uint32_t complexity);
    void buildComputeCommandBuffers(uint32_t buildMask);
    void destroyCommandBuffers();
    // ------------------------------------------------------------------


    void updateUniformBuffers();
    void draw(); 
    void prepare();
    virtual void render();
    virtual void viewChanged();
};

