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
        renderSystem->Render();
        auto& transform = ecsCoordinator.GetComponent<TransformComponent>(triangle);
        transform.yrot += 0.01f;
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

    triangle = ecsCoordinator.CreateEntity();
    ecsCoordinator.AddComponent(triangle, ModelComponent{model.get()});
    ecsCoordinator.AddComponent(triangle, TransformComponent{glm::vec3(0,0,10), 0.0f});
}





