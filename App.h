#include <memory>
#include <vulkan/vulkan.h>
#include "EvWindow.h"
#include "EvDevice.h"
#include "EvSwapchain.h"
#include "EvRastPipeline.h"
#include "ShaderTypes.h"

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
    Lol push = Lol {
        .offset = {0,0},
        .color = {1,1,1},
    };
    float time = 0;

    void createShaderModules();
    void createSwapchain();
    void createPipelineLayout();
    void createPipeline();
    void allocateCommandBuffers();
    void recordCommandBuffer(uint imageIndex);
    void drawFrame();
    void recreateSwapchain();

public:
    App();
    ~App();
    void Run();
};
