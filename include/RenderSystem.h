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
    std::unique_ptr<EvOverlay> overlay;

    void createShaderModules();
    void createSwapchain();
    void createDescriptorSetLayout();
    void createPipelineLayout();
    void createPipeline();
    void allocateCommandBuffers();
    void createOverlay();
    void recordCommandBuffer(uint32_t imageIndex, const EvCamera& camera) const;
    void recreateSwapchain();
public:
    RenderSystem(EvDevice& device);
    ~RenderSystem();

    void Render(const EvCamera &camera);

    Signature GetSignature() const override;
    std::unique_ptr<EvModel> createModel(const std::string& filename, const EvTexture* texture);
};
