#include <memory>
#include "EvWindow.h"
#include "EvDevice.h"
#include "EvSwapchain.h"
#include "EvRastPipeline.h"

#pragma once

class App {
    EvWindow window{640, 480, "My app"};
    EvDeviceInfo deviceInfo{};
    EvDevice device{deviceInfo, window};
    EvSwapchain swapchain{device};
    VkPipelineLayout vkPipelineLayout;
    EvRastPipelineInfo pipelineInfo{};
    std::unique_ptr<EvRastPipeline> rastPipeline{};

    void createPipelineLayout();
    void createPipeline();

public:
    App();
    ~App();
    void Run();
};
