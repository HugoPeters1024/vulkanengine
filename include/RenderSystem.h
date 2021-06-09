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
#include "EvGPass.h"
#include "EvComposePass.h"
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

    std::unique_ptr<EvGPass> gpass2;
    std::unique_ptr<EvComposePass> composepass2;

    void createShaderModules();
    void createSwapchain();
    void createDescriptorSetLayout();
    void createPipelineLayout();
    void createPipeline();

    void recordGBufferCommands(VkCommandBuffer commandBuffer, uint32_t imageIdx, const EvCamera &camera);

    void allocateCommandBuffers();
    void recordCommandBuffer(uint32_t imageIndex, const EvCamera &camera, EvOverlay *overlay);
    void recreateSwapchain();

public:
    RenderSystem(EvDevice& device);
    ~RenderSystem();

    void Render(const EvCamera &camera, EvOverlay* overlay);

    Signature GetSignature() const override;
    inline EvSwapchain* getSwapchain() const { assert(swapchain); return swapchain.get(); }
    std::unique_ptr<EvModel> createModel(const std::string& filename, const EvTexture* texture);
};
