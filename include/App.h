#pragma once
#include <memory>
#include <vulkan/vulkan.h>
#include "EvWindow.h"
#include "EvDevice.h"
#include "EvSwapchain.h"
#include "ShaderTypes.h"

#include <ecs/ecs.h>
#include "RenderSystem.h"
#include "EvInputHelper.h"
#include "PhysicsSystem.h"
#include "EvTexture.h"
#include "EvCamera.h"
#include "EvOverlay.h"

class App {
    EvWindow window;
    EvDevice device;
    EvInputHelper inputHelper;
    EvCamera camera;
    float time = 0;

    EcsCoordinator ecsCoordinator;
    std::shared_ptr<RenderSystem> renderSystem;
    std::shared_ptr<PhysicsSystem> physicsSystem;

    EvModel *cubeModel, *lucyModel, *florianModel;
    EvTexture *whiteTex;

    void createECSSystems();
    void loadModels();
    void createECSWorld();
    Entity addInstance(EvModel* model, rp3::BodyType bodyType, glm::vec3 scale, glm::vec3 position);

public:
    App();
    void Run();
};
