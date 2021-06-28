#pragma once

#include "../core.h"
#include "../EvDevice.h"

class PostPass {
    EvDevice& device;

    struct PostBuffer : public EvFrameBuffer {

    } framebuffer;

    VkShaderModule vertShader;
    VkShaderModule fragShader;
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkDescriptorSet> bloomDescriptorSets;

    VkSampler composedSampler;

    void createBuffer(uint32_t width, uint32_t height, uint32_t nrImages, VkFormat swapchainFormat,
                      const std::vector<VkImageView> &swapchainImageViews);
    void createDescriptorSetLayout();
    void allocateDescriptorSets(uint32_t nrImages);
    void createDescriptorSets(uint32_t nrImages, const std::vector<EvFrameBufferAttachment> &colorInputs, const std::vector<EvFrameBufferAttachment> &bloomInputs);
    void createPipeline();

public:
    PostPass(EvDevice &device, uint32_t width, uint32_t height, uint32_t nrImages, VkFormat swapchainFormat,
             const std::vector<VkImageView> &swapchainImageViews,
             const std::vector<EvFrameBufferAttachment> &colorInputs,
             const std::vector<EvFrameBufferAttachment> &bloomInputs);
    ~PostPass();

    inline VkRenderPass getRenderPass() const { return framebuffer.vkRenderPass; }

    void recreateFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages,
                             const std::vector<EvFrameBufferAttachment> &colorInputs,
                             const std::vector<EvFrameBufferAttachment> &bloomInputs, VkFormat swapchainFormat,
                             const std::vector<VkImageView> &swapchainImageViews);
    void beginPass(VkCommandBuffer commandBuffer, uint32_t imageIdx) const;
    void endPass(VkCommandBuffer commandBuffer) const;
};