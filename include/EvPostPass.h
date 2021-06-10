#pragma once

#include "EvDevice.h"
#include <vulkan/vulkan.h>
#include <VulkanInitializers.hpp>

class EvPostPass {
    EvDevice& device;

    struct PostBuffer : public EvFrameBuffer {

    } framebuffer;

    VkShaderModule vertShader;
    VkShaderModule fragShader;
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;

    VkSampler composedSampler;

    void createBuffer(uint32_t width, uint32_t height, uint32_t nrImages, VkFormat swapchainFormat,
                      const std::vector<VkImageView> &swapchainImageViews);
    void createDescriptorSetLayout();
    void allocateDescriptorSets(uint32_t nrImages);
    void createDescriptorSets(uint32_t nrImages, const std::vector<VkImageView> &composedViews);
    void createPipeline();

public:
    EvPostPass(EvDevice &device, uint32_t width, uint32_t height, uint32_t nrImages,
               VkFormat swapchainFormat, const std::vector<VkImageView> &swapchainImageViews,
               const std::vector<VkImageView> &composedImageViews);
    ~EvPostPass();

    inline VkRenderPass getRenderPass() const { return framebuffer.vkRenderPass; }

    void recreateFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages,
                             const std::vector<VkImageView> &composedViews, VkFormat swapchainFormat,
                             const std::vector<VkImageView> &swapchainImageViews);
    void beginPass(VkCommandBuffer commandBuffer, uint32_t imageIdx) const;
    void endPass(VkCommandBuffer commandBuffer) const;
};