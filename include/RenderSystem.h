#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "EvDevice.h"
#include "EvModel.h"
#include "EvRastPipeline.h"
#include "EvSwapchain.h"
#include "ecs/ecs.h"
#include <glm/glm.hpp>


struct ModelComponent {
    EvModel* model;
};

struct TransformComponent {
    glm::mat4 transform;
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

    void createShaderModules();
    void createSwapchain();
    void createPipelineLayout();
    void createPipeline();
    void allocateCommandBuffers();
    void recordCommandBuffer(uint32_t imageIndex) const;
    void recreateSwapchain();
public:
    RenderSystem(EvDevice& device);
    ~RenderSystem();

    void Render();

    Signature GetSignature() const override;
};
