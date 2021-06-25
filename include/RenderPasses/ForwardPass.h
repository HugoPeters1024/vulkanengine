#pragma once

#include "../core.h"
#include "../EvDevice.h"
#include "../ShaderTypes.h"
#include "../Primitives.h"
#include "../Components.h"

class ForwardPass : NoCopy
{
    EvDevice& device;

    struct Buffer : public EvFrameBuffer {
        std::vector<EvFrameBufferAttachment> colors;

        virtual void destroy(EvDevice& device) override {
            for(auto& color : colors) color.destroy(device);
            EvFrameBuffer::destroy(device);
        }
    } framebuffer;

    static const uint MAX_LIGHTS = 2000;
    struct LightBuffer {
        LightComponent lightData[MAX_LIGHTS];
        uint lightCount;
        float falloffConstant;
        float falloffLinear;
        float falloffQuadratic;
    };


    VkShaderModule vertShader;
    VkShaderModule fragShader;
    VkDescriptorSetLayout texturesDescriptorSetLayout;
    VkDescriptorSetLayout lightBufferDescriptorSetLayout;
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    std::vector<VkBuffer> lightBuffers;
    std::vector<VmaAllocation> lightBufferMemories;
    std::vector<LightBuffer*> lightBuffersMapped;
    std::vector<VkDescriptorSet> lightBufferDescriptorSets;

    void createFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages,
                           const std::vector<EvFrameBufferAttachment>& depthAttachments);
    void createTexturesDescriptorSetLayout();
    void createLightBufferDescriptorSetLayout();
    void createPipelineLayout();
    void createPipeline();
    void createLightBuffers(uint32_t nrImages);
    void allocateLightDescriptorSets(uint32_t nrImages);
    void createLightDescriptorSets(uint32_t nrImages);

public:
    ForwardPass(EvDevice& device, uint32_t width, uint32_t height, uint32_t nrImages, const std::vector<EvFrameBufferAttachment>& depthAttachments);
    ~ForwardPass();

    inline Buffer& getFramebuffer() { return framebuffer; }
    inline VkDescriptorSetLayout getDescriptorSetLayout() const { return texturesDescriptorSetLayout; }
    inline VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

    inline void setLightProperties(uint32_t imageIdx, float constant, float linear, float quadratic) {
        auto& buffer = *lightBuffersMapped[imageIdx];
        buffer.falloffConstant = constant;
        buffer.falloffLinear = linear;
        buffer.falloffQuadratic = quadratic;
    }

    void updateLights(LightComponent* lightData, uint32_t nrLights, uint32_t imageIdx);
    void recreateFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages, const std::vector<EvFrameBufferAttachment>& depthAttachments);
    void startPass(VkCommandBuffer cmdBuffer, uint32_t imageIdx) const;
    void endPass(VkCommandBuffer cmdBuffer) const;
};
