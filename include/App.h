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

    Entity floor;
    std::vector<Entity> lights;
    EvMesh* cubeMesh;

    void createECSSystems();
    void createWorld();
    Entity addInstance(EvMesh* mesh, rp3::BodyType bodyType, glm::vec3 scale, glm::vec3 position, glm::vec2 textureScale, TextureSet* textureSet = nullptr);

public:
    App();
    void Run();
};
