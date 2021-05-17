#include <memory>
#include <vulkan/vulkan.h>
#include "EvWindow.h"
#include "EvDevice.h"
#include "EvSwapchain.h"
#include "EvRastPipeline.h"
#include "ShaderTypes.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "ecs/ecs.h"
#include "RenderSystem.h"

#pragma once

class App {
    EvWindow window = EvWindow(640, 480, "My app");
    EvDeviceInfo deviceInfo{};
    EvDevice device{deviceInfo, window};
    std::unique_ptr<EvSwapchain> swapchain;
    VkPipelineLayout vkPipelineLayout;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    EvRastPipelineInfo pipelineInfo{};
    std::unique_ptr<EvRastPipeline> rastPipeline{};
    std::vector<VkCommandBuffer> commandBuffers{};
    std::unique_ptr<EvModel> model{};
    float time = 0;

    EcsCoordinator ecsCoordinator;
    std::shared_ptr<RenderSystem> renderSystem;

    void createShaderModules();
    void createSwapchain();
    void createPipelineLayout();
    void createPipeline();
    void createECS();
    void allocateCommandBuffers();
    void loadModel();
    void recordCommandBuffer(uint imageIndex);
    void drawFrame();
    void recreateSwapchain();

public:
    App();
    ~App();
    void Run();
};
