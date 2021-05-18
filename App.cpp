#include "App.h"
#include "utils.h"
#include "EvModel.h"
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
        transform.transform = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0,0,0.5f)), time, glm::vec3(0,1,0));
        time += 0.01f;
    }

    printf("Flushing GPU before shutdown...\n");
    vkDeviceWaitIdle(device.vkDevice);
    printf("Goodbye!\n");
}


void App::loadModel() {
    std::vector<Vertex> vertices = {
            {{0.0f, -0.5f, 0.0f}},
            {{-0.5f, 0.5f, 0.0f}},
            {{0.5f, 0.5f, 0.0f}},
    };
    model = std::make_unique<EvModel>(device, vertices);
}

void App::createECS() {
    ecsCoordinator.RegisterComponent<ModelComponent>();
    ecsCoordinator.RegisterComponent<TransformComponent>();
    renderSystem = ecsCoordinator.RegisterSystem<RenderSystem>(device);

    triangle = ecsCoordinator.CreateEntity();
    ecsCoordinator.AddComponent(triangle, ModelComponent{model.get()});
    ecsCoordinator.AddComponent(triangle, TransformComponent{glm::mat4(1.0f)});
}





