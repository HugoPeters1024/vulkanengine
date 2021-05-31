#include <memory>
#include <vulkan/vulkan.h>
#include "EvWindow.h"
#include "EvDevice.h"
#include "EvSwapchain.h"
#include "EvRastPipeline.h"
#include "ShaderTypes.h"

#include <ecs/ecs.h>
#include "RenderSystem.h"
#include "InputSystem.h"
#include "EvTexture.h"

#pragma once

class App {
    EvWindow window = EvWindow(640, 480, "My app");
    EvDeviceInfo deviceInfo{};
    EvDevice device{deviceInfo, window};
    std::unique_ptr<EvModel> model{};
    float time = 0;
    Entity cube;

    EcsCoordinator ecsCoordinator;
    std::shared_ptr<RenderSystem> renderSystem;
    std::shared_ptr<InputSystem> inputSystem;

    std::unique_ptr<EvTexture> texture, whiteTex;

    void createECSSystems();
    void loadModel();
    void createECSWorld();

public:
    App();
    void Run();
};
