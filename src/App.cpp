#include "App.h"

EvDeviceInfo deviceInfo {
};

App::App()
    : window(1280, 768, "Hello world")
    , device(deviceInfo, window)
    , inputHelper(window.glfwWindow)
{
    camera = EvCamera {
        .fov = 90,
    };
    createECSSystems();
    createWorld();
}

void App::Run() {
    uint tick = 0;
    while (!window.shouldClose()) {
        tick++;
        double startFrame = glfwGetTime();
        window.processEvents();
        inputHelper.swapBuffers();
        camera.handleInput(inputHelper);
        physicsSystem->Update();
        renderSystem->Render(camera);
        time += 0.01f;
        double timePerFrame = glfwGetTime() - startFrame;
        auto& uiinfo = renderSystem->getUIInfo();
        uiinfo.fps = static_cast<float>(1.0 / timePerFrame);

        physicsSystem->setWorldGravity(renderSystem->getUIInfo().gravity);
        auto& floorModel = ecsCoordinator.GetComponent<ModelComponent>(floor);
        floorModel.textureScale = glm::vec2(uiinfo.floorScale);

        if (tick % 10 == 0) {
            addInstance(cubeMesh, rp3::BodyType::DYNAMIC, glm::vec3(0.3f), glm::vec3(0, 0, 6), glm::vec2(1.0f));
        }
    }

    printf("Flushing GPU before shutdown...\n");
    vkDeviceWaitIdle(device.vkDevice);
    printf("Goodbye!\n");
}

void App::createECSSystems() {
    ecsCoordinator.RegisterComponent<ModelComponent>();
    renderSystem = ecsCoordinator.RegisterSystem<RenderSystem>(device);

    ecsCoordinator.RegisterComponent<PhysicsComponent>();
    physicsSystem = ecsCoordinator.RegisterSystem<PhysicsSystem>();
}

void App::createWorld() {
    auto terracottaTex = renderSystem->createTextureFromFile("assets/textures/terracotta.jpg");
    cubeMesh = renderSystem->loadMesh("assets/models/cube.obj");
    auto lucyMesh = renderSystem->loadMesh("assets/models/lucy.obj");

    std::string diffuseTexFile, normalTexFile;
    auto florianMesh = renderSystem->loadMesh("assets/models/florian_small.obj", &diffuseTexFile);
    auto florianTex = renderSystem->createTextureFromFile(diffuseTexFile);
    auto florianTexSet = renderSystem->createTextureSet(florianTex);

    addInstance(cubeMesh, rp3::BodyType::DYNAMIC, glm::vec3(0.3f), glm::vec3(0, 0, 6), glm::vec2(1.0f, 1.0f));
    addInstance(cubeMesh, rp3::BodyType::DYNAMIC, glm::vec3(0.3f), glm::vec3(0.2, -1, 6.1), glm::vec2(1.0f, 1.0f));
    auto lucy = addInstance(lucyMesh, rp3::BodyType::DYNAMIC, glm::vec3(0.1f), glm::vec3(0.2, -3, 6.01), glm::vec2(1.0f));
    auto florian = addInstance(florianMesh, rp3::BodyType::DYNAMIC, glm::vec3(0.1f), glm::vec3(2.2, -3, 6.01), glm::vec2(1.0f), florianTexSet);
    physicsSystem->setMass(florian, 10);
    floor = addInstance(cubeMesh, rp3::BodyType::STATIC, glm::vec3(25.0f, 0.2f, 25.0f), glm::vec3(0, 4, 6), glm::vec2(8.0f), renderSystem->createTextureSet(terracottaTex));
    physicsSystem->setMass(lucy, 10);
   // physicsSystem->setAngularVelocity(floor, glm::vec3(1.f,0,  0));
}

Entity App::addInstance(EvMesh *mesh, rp3::BodyType bodyType, glm::vec3 scale, glm::vec3 position, glm::vec2 textureScale, TextureSet *textureSet) {
    Entity entity = ecsCoordinator.CreateEntity();
    ecsCoordinator.AddComponent<ModelComponent>(entity, {
        .mesh = mesh,
        .textureSet = textureSet,
        .scale = scale,
        .textureScale = textureScale,
    });

    ecsCoordinator.AddComponent<PhysicsComponent>(entity, {
        .rigidBody = physicsSystem->createRigidBody(bodyType, position),
    });

    physicsSystem->addIntersectionBoxBody(entity, mesh->boundingBox * scale);
    physicsSystem->linkModelComponent(entity);
    return entity;
}