#pragma once

#include "../EvCamera.h"
#include "../core.h"
#include "../EvDevice.h"
#include "../ShaderTypes.h"
#include "../Primitives.h"
#include "../Components.h"
#include "../UBOBuffer.h"

class ForwardPass : NoCopy
{
    EvDevice& device;

    struct Buffer : public EvFrameBuffer {
        std::vector<EvFrameBufferAttachment> colors;
        std::vector<EvFrameBufferAttachment> blooms;

        virtual void destroy(EvDevice& device) override {
            for(auto& color : colors) color.destroy(device);
            for(auto& bloom : blooms) bloom.destroy(device);
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
    std::unique_ptr<UBOBuffer<LightBuffer>> lightUBO;

    struct Skybox {
        struct {
            glm::mat4 camera;
        } push;
        VkShaderModule vertShader;
        VkShaderModule fragShader;
        VkDescriptorSetLayout descriptorSetLayout;
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;

        inline void destroy(EvDevice& device) {
            vkDestroyShaderModule(device.vkDevice, vertShader, nullptr);
            vkDestroyShaderModule(device.vkDevice, fragShader, nullptr);
            vkDestroyDescriptorSetLayout(device.vkDevice, descriptorSetLayout, nullptr);
            vkDestroyPipelineLayout(device.vkDevice, pipelineLayout, nullptr);
            vkDestroyPipeline(device.vkDevice, pipeline, nullptr);
        }
    } skybox;

    void createFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages,
                           const std::vector<EvFrameBufferAttachment>& depthAttachments);
    void createTexturesDescriptorSetLayout();
    void createLightBufferDescriptorSetLayout();
    void createPipelineLayout();
    void createPipeline();
    void createLightBuffers(uint32_t nrImages);

    void createSkyboxDescriptorSetLayout();
    void createSkyboxPipelineLayout();
    void createSkyboxPipeline();

public:
    ForwardPass(EvDevice& device, uint32_t width, uint32_t height, uint32_t nrImages, const std::vector<EvFrameBufferAttachment>& depthAttachments);
    ~ForwardPass();

    inline Buffer& getFramebuffer() { return framebuffer; }
    inline VkDescriptorSetLayout getDescriptorSetLayout() const { return texturesDescriptorSetLayout; }
    inline VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
    inline Skybox& getSkybox() { return skybox; }

    inline void setLightProperties(uint32_t imageIdx, float constant, float linear, float quadratic) {
        auto& buffer = *lightUBO->getPtr(imageIdx);
        buffer.falloffConstant = constant;
        buffer.falloffLinear = linear;
        buffer.falloffQuadratic = quadratic;
    }

    inline void bindSkyboxPipeline(VkCommandBuffer cmdBuffer, const EvCamera& camera) {
        skybox.push.camera = camera.getVPMatrix(device.window.getAspectRatio());
        vkCmdPushConstants(cmdBuffer, skybox.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(skybox.push), &skybox.push);
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skybox.pipeline);
    }

    void updateLights(LightComponent* lightData, uint32_t nrLights, uint32_t imageIdx);
    void recreateFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages, const std::vector<EvFrameBufferAttachment>& depthAttachments);
    void startPass(VkCommandBuffer cmdBuffer, uint32_t imageIdx) const;
    void endPass(VkCommandBuffer cmdBuffer) const;
};
