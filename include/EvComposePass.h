#pragma once

#include "core.h"
#include "EvDevice.h"
#include "EvCamera.h"

class EvComposePass {
private:
    EvDevice& device;
    struct ComposeBuffer : public EvFrameBuffer {
        std::vector<EvFrameBufferAttachment> composeds;

        virtual void destroy(EvDevice& device) override {
            for(auto& composed : composeds) composed.destroy(device);
            EvFrameBuffer::destroy(device);
        }
    } framebuffer;

    VkShaderModule vertShader;
    VkShaderModule fragShader;
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    VkDescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;

    VkSampler normalSampler;
    VkSampler posSampler;
    VkSampler albedoSampler;

    void createBuffer(uint32_t width, uint32_t height, uint32_t nrImages);
    void createDescriptorSetLayout();
    void allocateDescriptorSets(uint32_t nrImages);
    void createDescriptorSets(uint32_t nrImages, const std::vector<VkImageView>& posViews, const std::vector<VkImageView>& albedoViews);
    void createPipeline();

public:
    EvComposePass(EvDevice &device, uint32_t width, uint32_t height, uint32_t nrImages,
                  const std::vector<VkImageView>& posViews,
                  const std::vector<VkImageView>& albedoViews);
    ~EvComposePass();

    inline VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
    inline std::vector<VkImageView> getComposedViews() const {
        std::vector<VkImageView> ret{};
        for(const auto& composed : framebuffer.composeds) {
            ret.push_back(composed.view);
        }
        return ret;
    }
    inline std::vector<EvFrameBufferAttachment> getComposedAttachments() const { return framebuffer.composeds; }

    void recreateFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages, const std::vector<VkImageView>& posViews, const std::vector<VkImageView>& albedoViews);
    void render(VkCommandBuffer cmdBuffer, uint32_t imageIdx, const EvCamera& camera) const;
};
