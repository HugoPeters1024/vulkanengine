#pragma once

#include <vulkan/vulkan.h>
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
    glm::mat4 transform{glm::mat4(1.0f)};
};

class RenderSystem : public System
{
    EvDevice& device;
    std::unique_ptr<EvSwapchain> swapchain;
    std::vector<VkCommandBuffer> commandBuffers;
    VkPipelineLayout vkPipelineLayout;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    EvRastPipelineInfo pipelineInfo{};
    std::unique_ptr<EvRastPipeline> rastPipeline{};
    VkDescriptorSetLayout vkDescriptorSetLayout;

    struct GBuffer : public EvFrameBuffer {
        EvFrameBufferAttachment albedo;
    } gbuffer;

    void createShaderModules();
    void createSwapchain();
    void createDescriptorSetLayout();
    void createPipelineLayout();
    void createPipeline();
    void createGBuffer();
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
