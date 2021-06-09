#pragma once

#include <vulkan/vulkan.h>
#include <VulkanInitializers.hpp>
#include <vector>
#include "EvDevice.h"
#include "EvModel.h"
#include "EvCamera.h"
#include "EvRastPipeline.h"
#include "EvSwapchain.h"
#include "ShaderTypes.h"
#include "EvOverlay.h"
#include <ecs/ecs.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>

struct ModelComponent {
    EvModel* model{};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    glm::mat4 transform{1.0f};
};

class RenderSystem : public System
{
    EvDevice& device;
    std::unique_ptr<EvSwapchain> swapchain;
    std::vector<VkCommandBuffer> commandBuffers;
    VkPipelineLayout vkPipelineLayout;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    std::unique_ptr<EvRastPipeline> rastPipeline{};
    VkDescriptorSetLayout vkDescriptorSetLayout;

    struct {
        struct GBuffer : public EvFrameBuffer {
            EvFrameBufferAttachment normal;
            EvFrameBufferAttachment position;
            EvFrameBufferAttachment albedo;
            EvFrameBufferAttachment depth;

            virtual void destroy(EvDevice& device) override {
                normal.destroy(device);
                position.destroy(device);
                albedo.destroy(device);
                depth.destroy(device);
                EvFrameBuffer::destroy(device);
            }
        } framebuffer;
        VkShaderModule vertShader;
        VkShaderModule fragShader;
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
        VkDescriptorSetLayout descriptorSetLayout;
        VkCommandBuffer commandBuffer;
    } gpass;

    struct {
        struct ComposeBuffer : public EvFrameBuffer {
            EvFrameBufferAttachment composed;

            virtual void destroy(EvDevice& device) override {
                composed.destroy(device);
                EvFrameBuffer::destroy(device);
            }
        } framebuffer;
        VkShaderModule vertShader;
        VkShaderModule fragShader;
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet descriptorSet;
        VkCommandBuffer commandBuffer;

        VkSampler normalSampler;
        VkSampler posSampler;
        VkSampler albedoSampler;
    } composepass;

    void createShaderModules();
    void createSwapchain();
    void createDescriptorSetLayout();
    void createPipelineLayout();
    void createPipeline();

    void createGBuffer();
    void createGBufferDescriptorSetLayout();
    void createGBufferPipeline();
    void allocateGBufferCommandBuffer();
    void recordGBufferCommands(const EvCamera &camera);

    void createComposeBuffer();
    void createComposeDescriptorSetLayout();
    void allocateComposeDescriptorSet();
    void createComposeDescriptorSet();
    void createComposePipeline();
    void allocateComposeCommandBuffer();
    void recordComposeCommands();


    void allocateCommandBuffers();
    void recordCommandBuffer(uint32_t imageIndex, const EvCamera &camera, EvOverlay *overlay) const;
    void recreateSwapchain();

public:
    RenderSystem(EvDevice& device);
    ~RenderSystem();

    void Render(const EvCamera &camera, EvOverlay* overlay);

    Signature GetSignature() const override;
    inline EvSwapchain* getSwapchain() const { assert(swapchain); return swapchain.get(); }
    std::unique_ptr<EvModel> createModel(const std::string& filename, const EvTexture* texture);
};
