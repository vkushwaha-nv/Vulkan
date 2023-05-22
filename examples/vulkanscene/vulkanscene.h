#define ENABLE_VALIDATION true

#define MAX_DRAW_FRAMES 0 // 0 = INF
#define RANDOM_BUFFER_SIZE (512 * 1024 * 1024ULL)
#define SWAP_CHAIN_IMAGE_COUNT 3

#define QUEUE_GRAPHICS 0
#define QUEUE_COMPUTE  1
#define QUEUE_COPY     2

static unsigned currentFrameCounter = 0;

class VulkanExample : public VulkanExampleBase
{
public:
    struct DemoModel
    {
        vkglTF::Model* glTF;
        VkPipeline *pipeline;
    };
    std::vector<DemoModel> demoModels;

    struct {
        vks::Buffer meshVS;
    } uniformData;

    struct {
        glm::mat4 projection;
        glm::mat4 model;
        glm::mat4 normal;
        glm::mat4 view;
        glm::vec4 lightPos;
    } uboVS;

    struct
    {
        vks::TextureCubeMap skybox;
    } textures;

    struct {
        VkPipeline logos;
        VkPipeline models;
        VkPipeline skybox;
    } pipelines;

    VkPipelineLayout pipelineLayout;
    VkDescriptorSet descriptorSet;
    VkDescriptorSetLayout descriptorSetLayout;

    glm::vec4 lightPos = glm::vec4(1.0f, 4.0f, 0.0f, 0.0f);

    // ----VKKK------------------------------------------------------
    // 5 random buffers
    struct {
        vks::Buffer buffer1;
        vks::Buffer buffer2;
        vks::Buffer buffer3;
        vks::Buffer buffer4;
        vks::Buffer buffer5;
    } randomBuffers;

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
    void buildDefaultCommandBuffers();
    
    void setupDescriptorPool();
    void setupDescriptorSetLayout();
    void setupDescriptorSet();
    void preparePipelines();
    
    // Prepare and initialize uniform buffer containing shader uniforms
    void prepareUniformBuffers(); 

    // ----VKKK------------------------------------------------------
    void createSBOBuffers();
    void createCommandPoolAndBuffers();
    void buildOneTimeSubmitCommandBuffers();
    void buildTransferCommandBuffers(uint32_t buildMask);
    void buildComputeCommandBuffers(uint32_t buildMask);
    void destroyCommandBuffers();
    // ------------------------------------------------------------------



    void updateUniformBuffers();
    void draw(); 
    void prepare();
    virtual void render();
    virtual void viewChanged();
};

