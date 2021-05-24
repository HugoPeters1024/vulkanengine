#include "App.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>

App::App() {
    loadModel();
    createECS();
}

void App::Run() {
    while (!window.shouldClose()) {
        window.processEvents();
        inputSystem->Update();
        renderSystem->Render();
        auto& transform = ecsCoordinator.GetComponent<TransformComponent>(triangle);
        transform.rotation.y += 0.01f;
        transform.rotation.z += 0.01515926f;
        time += 0.01f;
    }

    printf("Flushing GPU before shutdown...\n");
    vkDeviceWaitIdle(device.vkDevice);
    printf("Goodbye!\n");
}


void App::loadModel() {
    model = std::make_unique<EvModel>(device, "cube.obj");
}

void App::createECS() {
    ecsCoordinator.RegisterComponent<ModelComponent>();
    ecsCoordinator.RegisterComponent<TransformComponent>();
    renderSystem = ecsCoordinator.RegisterSystem<RenderSystem>(device);

    ecsCoordinator.RegisterComponent<InputComponent>();
    inputSystem = ecsCoordinator.RegisterSystem<InputSystem>(window.glfwWindow);

    triangle = ecsCoordinator.CreateEntity();
    ecsCoordinator.AddComponent(triangle, ModelComponent{model.get()});
    ecsCoordinator.AddComponent(triangle, TransformComponent{glm::vec3(0,0,10), glm::vec3(0.0f)});
    ecsCoordinator.AddComponent(triangle, InputComponent {
        .handler = [this](const EvInputHelper& inputHelper) {
            auto &transform = ecsCoordinator.GetComponent<TransformComponent>(triangle);
            if (inputHelper.isDown(GLFW_KEY_RIGHT)) transform.position.x -= 0.05f;
            if (inputHelper.isDown(GLFW_KEY_LEFT)) transform.position.x += 0.05f;
        },
    });
}





