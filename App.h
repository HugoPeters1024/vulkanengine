#include <memory>
#include <vulkan/vulkan.h>
#include "EvWindow.h"
#include "EvDevice.h"
#include "EvSwapchain.h"
#include "EvRastPipeline.h"

#pragma once

class App {
    EvWindow window{640, 480, "My app"};
    EvDeviceInfo deviceInfo{};
    EvDevice device{deviceInfo, window};
    std::unique_ptr<EvSwapchain> swapchain;
    VkPipelineLayout vkPipelineLayout;
    EvRastPipelineInfo pipelineInfo{};
    std::unique_ptr<EvRastPipeline> rastPipeline{};
    std::vector<VkCommandBuffer> commandBuffers{};

    void createSwapchain();
    void createPipelineLayout();
    void createPipeline();
    void allocateCommandBuffers();
    void recordCommandBuffers();
    void drawFrame();
    void recreateSwapchain();

public:
    App();
    ~App();
    void Run();
};
