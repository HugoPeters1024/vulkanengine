#include "App.h"

App::App() {
    createECSSystems();
    loadModel();
    createECSWorld();
}

void App::Run() {
    while (!window.shouldClose()) {
        window.processEvents();
        inputSystem->Update();
        physicsSystem->Update();
        renderSystem->Render();
        time += 0.01f;
    }

    printf("Flushing GPU before shutdown...\n");
    vkDeviceWaitIdle(device.vkDevice);
    printf("Goodbye!\n");
}

void App::createECSSystems() {
    ecsCoordinator.RegisterComponent<ModelComponent>();
    renderSystem = ecsCoordinator.RegisterSystem<RenderSystem>(device);

    ecsCoordinator.RegisterComponent<InputComponent>();
    inputSystem = ecsCoordinator.RegisterSystem<InputSystem>(window.glfwWindow);

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
    physicsSystem->setAngularVelocity(floor, glm::vec3(0, 1, 0));
    /*
    ecsCoordinator.AddComponent(cube, InputComponent {
            .handler = [this](const EvInputHelper& inputHelper) {
                //auto &transform = ecsCoordinator.GetComponent<TransformComponent>(cube);
                //if (inputHelper.isDown(GLFW_KEY_RIGHT)) transform.position.x -= 0.05f;
                //if (inputHelper.isDown(GLFW_KEY_LEFT)) transform.position.x += 0.05f;
                //if (inputHelper.isDown(GLFW_KEY_UP)) transform.position.z += 0.05f;
                //if (inputHelper.isDown(GLFW_KEY_DOWN)) transform.position.z -= 0.05f;
            },
    });
     */
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
