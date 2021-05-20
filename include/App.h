#include <memory>
#include <vulkan/vulkan.h>
#include "EvWindow.h"
#include "EvDevice.h"
#include "EvSwapchain.h"
#include "EvRastPipeline.h"
#include "ShaderTypes.h"

#include "ecs/ecs.h"
#include "RenderSystem.h"

#pragma once

class App {
    EvWindow window = EvWindow(640, 480, "My app");
    EvDeviceInfo deviceInfo{};
    EvDevice device{deviceInfo, window};
    std::unique_ptr<EvModel> model{};
    float time = 0;
    Entity triangle;

    EcsCoordinator ecsCoordinator;
    std::shared_ptr<RenderSystem> renderSystem;

    void createECS();
    void loadModel();

public:
    App();
    void Run();
};
