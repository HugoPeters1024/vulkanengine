#pragma once

#include "core.h"
#include "EvDevice.h"
#include "EvCamera.h"
#include "EvMesh.h"
#include "Components.h"

struct ComposePush {
    glm::mat4 camera;
    glm::vec4 camPos;
    glm::vec4 invScreenSizeAttenuation;
};

struct SkyPush {
    glm::mat4 camera;
};

struct ForwardPush {
    glm::mat4 camera;
    glm::mat4 mvp;
};

class EvComposePass {
private:
    EvDevice& device;

    void createFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages, const std::vector<EvFrameBufferAttachment>& depthAttachments);
    void createDescriptorSetLayout();
    void allocateDescriptorSets(uint32_t nrImages);
    void createDescriptorSets(uint32_t nrImages, const std::vector<VkImageView> &posViews,
                              const std::vector<VkImageView> &normalViews,
                              const std::vector<VkImageView> &albedoViews);
    void createLightPipeline();

    void createSkyDescriptorSetLayout();
    void createSkyPipeline();

    void createForwardDescriptorSetLayout();
    void createForwardPipeline();


public:
    static const uint MAX_LIGHTS = 1000;
    struct ComposeBuffer : public EvFrameBuffer {
        std::vector<EvFrameBufferAttachment> composeds;

        virtual void destroy(EvDevice& device) override {
            for(auto& composed : composeds) composed.destroy(device);
            EvFrameBuffer::destroy(device);
        }
    } framebuffer;

    struct {
        VkShaderModule vertShader;
        VkShaderModule fragShader;
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;

        VkDescriptorSetLayout descriptorSetLayout;
        std::vector<VkDescriptorSet> descriptorSets;

        VkSampler normalSampler;
        VkSampler posSampler;
        VkSampler albedoSampler;

        std::vector<VkBuffer> lightDataBuffers;
        std::vector<VmaAllocation> lightDataBufferMemories;
        std::vector<void*> lightDataMapped;
    } lightPipeline;

    struct {
        VkShaderModule vertShader;
        VkShaderModule fragShader;
        VkDescriptorSetLayout descriptorSetLayout;
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
    } skyPipeline;

    struct {
        VkShaderModule vertShader;
        VkShaderModule fragShader;
        VkDescriptorSetLayout descriptorSetLayout;
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
    } forwardPipeline;

    EvComposePass(EvDevice &device, uint32_t width, uint32_t height, uint32_t nrImages,
                  const std::vector<VkImageView> &posViews, const std::vector<VkImageView> &normalViews,
                  const std::vector<VkImageView> &albedoViews,
                  const std::vector<EvFrameBufferAttachment> &depthAttachments);
    ~EvComposePass();

    inline VkPipelineLayout getLightPipelineLayout() const { return lightPipeline.pipelineLayout; }
    inline std::vector<VkImageView> getComposedViews() const {
        std::vector<VkImageView> ret{};
        for(const auto& composed : framebuffer.composeds) {
            ret.push_back(composed.view);
        }
        return ret;
    }
    inline std::vector<EvFrameBufferAttachment> getComposedAttachments() const { return framebuffer.composeds; }

    void recreateFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages,
                             const std::vector<VkImageView> &posViews,
                             const std::vector<VkImageView> &normalViews,
                             const std::vector<VkImageView> &albedoViews,
                             const std::vector<EvFrameBufferAttachment> &depthAttachments);
    void beginPass(VkCommandBuffer commandBuffer, uint32_t imageIdx, const EvCamera& camera) const;
    void endPass(VkCommandBuffer commandBuffer);
    void bindLightPipeline(VkCommandBuffer commandBuffer, uint32_t imageIdx) const;
    void bindSkyPipeline(VkCommandBuffer commandBuffer) const;
    void bindForwardPipeline(VkCommandBuffer commandBuffer) const;
    void updateLights(LightComponent* data, uint nrLights, uint32_t imageIdx);
};
