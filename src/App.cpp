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
        auto& transform = ecsCoordinator.GetComponent<TransformComponent>(cube);
        transform.rotation.y += 0.01f;
        //transform.rotation.z += 0.01515926f;
        time += 0.01f;
    }

    printf("Flushing GPU before shutdown...\n");
    vkDeviceWaitIdle(device.vkDevice);
    printf("Goodbye!\n");
}

void App::createECSSystems() {
    ecsCoordinator.RegisterComponent<ModelComponent>();
    ecsCoordinator.RegisterComponent<TransformComponent>();
    renderSystem = ecsCoordinator.RegisterSystem<RenderSystem>(device);

    ecsCoordinator.RegisterComponent<InputComponent>();
    inputSystem = ecsCoordinator.RegisterSystem<InputSystem>(window.glfwWindow);

    ecsCoordinator.RegisterComponent<PhysicsComponent>();
    physicsSystem = ecsCoordinator.RegisterSystem<PhysicsSystem>();
}

void App::loadModel() {
    texture = EvTexture::fromFile(device, "assets/textures/cube.png");
    whiteTex = EvTexture::fromIntColor(device, 0xffffff);
    model = renderSystem->createModel("assets/models/cube.obj", whiteTex.get());
}

void App::createECSWorld() {
    cube = ecsCoordinator.CreateEntity();
    ecsCoordinator.AddComponent(cube, ModelComponent{model.get()});
    ecsCoordinator.AddComponent(cube, TransformComponent{glm::vec3(0, 0, 6), glm::vec3(0.0f), glm::vec3(0.1f)});
    ecsCoordinator.AddComponent(cube, InputComponent {
            .handler = [this](const EvInputHelper& inputHelper) {
                auto &transform = ecsCoordinator.GetComponent<TransformComponent>(cube);
                if (inputHelper.isDown(GLFW_KEY_RIGHT)) transform.position.x -= 0.05f;
                if (inputHelper.isDown(GLFW_KEY_LEFT)) transform.position.x += 0.05f;
                if (inputHelper.isDown(GLFW_KEY_UP)) transform.position.z += 0.05f;
                if (inputHelper.isDown(GLFW_KEY_DOWN)) transform.position.z -= 0.05f;
            },
    });
    ecsCoordinator.AddComponent(cube, PhysicsComponent {
        .rigidBody = physicsSystem->cubeBody(glm::vec3(1.0f)),
    });
}
