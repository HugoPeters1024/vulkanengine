#pragma once
#include <memory>
#include <vulkan/vulkan.h>
#include "EvWindow.h"
#include "EvDevice.h"
#include "EvSwapchain.h"
#include "EvRastPipeline.h"
#include "ShaderTypes.h"

#include <ecs/ecs.h>
#include "RenderSystem.h"
#include "EvInputHelper.h"
#include "PhysicsSystem.h"
#include "EvTexture.h"
#include "EvCamera.h"

class App {
    EvWindow window = EvWindow(640, 480, "My app");
    EvDeviceInfo deviceInfo{};
    EvDevice device{deviceInfo, window};
    EvCamera camera;
    EvInputHelper inputHelper{window.glfwWindow};
    std::unique_ptr<EvModel> cubeModel, lucyModel;
    float time = 0;
    Entity cube;

    EcsCoordinator ecsCoordinator;
    std::shared_ptr<RenderSystem> renderSystem;
    std::shared_ptr<PhysicsSystem> physicsSystem;

    std::unique_ptr<EvTexture> texture, whiteTex;

    void createECSSystems();
    void loadModel();
    void createECSWorld();
    Entity addInstance(EvModel* model, rp3::BodyType bodyType, glm::vec3 scale, glm::vec3 position);

public:
    App();
    void Run();
};
