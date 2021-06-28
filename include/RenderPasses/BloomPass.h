#pragma once

#include "../core.h"
#include "../EvDevice.h"

class BloomPass {
    EvDevice& device;
    const std::vector<EvFrameBufferAttachment> bloomAttachments;

    struct Buffer {
        uint32_t width, height;
        std::vector<VkImage> tmpImages;
        std::vector<VmaAllocation> tmpImageMemory;
        std::vector<VkImageView> tmpImageViews;

        void destroy(EvDevice& device) {
            for(int i=0; i<tmpImages.size(); i++) {
                vmaDestroyImage(device.vmaAllocator, tmpImages[i], tmpImageMemory[i]);
                vkDestroyImageView(device.vkDevice, tmpImageViews[i], nullptr);
            }
        }
    } framebuffer;

    struct Push {
        glm::ivec4 d;
    };

    VkShaderModule compShader;

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkDescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSet> horzDescriptorSets;
    std::vector<VkDescriptorSet> vertDescriptorSets;

    void createFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages, const std::vector<EvFrameBufferAttachment>& inputs);
    void createDescriptorSetLayout();
    void createPipelineLayout();
    void createPipeline();
    void allocateDescriptorSets(uint32_t nrImages);
    void createDescriptorSets(uint32_t nrImages, const std::vector<EvFrameBufferAttachment> &inputs);

public:
    BloomPass(EvDevice& device, uint32_t width, uint32_t height, uint32_t nrImages, const std::vector<EvFrameBufferAttachment>& inputs);
    ~BloomPass();

    inline Buffer& getFramebuffer() { return framebuffer; }

    void recreateFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages, const std::vector<EvFrameBufferAttachment>& inputs);
    void run(VkCommandBuffer cmdBuffer, uint32_t imageIdx) const;
};