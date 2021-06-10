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
    loadModel();
    createECSWorld();
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
        //physicsSystem->setWorldGravity(overlay->uiInfo.gravity);

        if (tick % 60 == 0) {
            addInstance(cubeModel.get(), rp3::BodyType::DYNAMIC, glm::vec3(0.3f), glm::vec3(0, 0, 6));
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

void App::loadModel() {
    texture = EvTexture::fromFile(device, "assets/textures/cube.png");
    whiteTex = EvTexture::fromIntColor(device, 0xffffff);
    cubeModel = renderSystem->createModel("assets/models/cube.obj", whiteTex.get());
    lucyModel = renderSystem->createModel("assets/models/lucy.obj", whiteTex.get());
}

void App::createECSWorld() {
    addInstance(cubeModel.get(), rp3::BodyType::DYNAMIC, glm::vec3(0.3f), glm::vec3(0, 0, 6));
    addInstance(cubeModel.get(), rp3::BodyType::DYNAMIC, glm::vec3(0.3f), glm::vec3(0.2, -1, 6.1));
    auto lucy = addInstance(lucyModel.get(), rp3::BodyType::DYNAMIC, glm::vec3(0.03f), glm::vec3(0.2, -3, 6.01));
    auto floor = addInstance(cubeModel.get(), rp3::BodyType::KINEMATIC, glm::vec3(5.0f, 0.2f, 5.0f), glm::vec3(0, 4, 6));
    physicsSystem->setMass(lucy, 10);
   // physicsSystem->setAngularVelocity(floor, glm::vec3(1.f,0,  0));
}

Entity App::addInstance(EvModel *model, rp3::BodyType bodyType, glm::vec3 scale, glm::vec3 position) {
    Entity entity = ecsCoordinator.CreateEntity();
    ecsCoordinator.AddComponent<ModelComponent>(entity, {
        .model = model,
        .scale = scale,
    });

    ecsCoordinator.AddComponent<PhysicsComponent>(entity, {
        .rigidBody = physicsSystem->createRigidBody(bodyType, position),
    });

    physicsSystem->addIntersectionBoxBody(entity, model->boundingBox * scale);
    physicsSystem->linkModelComponent(entity);
    return entity;
}