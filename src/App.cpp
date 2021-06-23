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
        auto& uiinfo = renderSystem->getUIInfo();

        window.processEvents();
        inputHelper.swapBuffers();
        camera.handleInput(inputHelper);
        physicsSystem->Update(uiinfo.forceField);
        renderSystem->Render(camera);
        time += 0.01f;
        double timePerFrame = glfwGetTime() - startFrame;
        uiinfo.fps = static_cast<float>(1.0 / timePerFrame);

        physicsSystem->setWorldGravity(renderSystem->getUIInfo().gravity);
        auto& floorModel = ecsCoordinator.GetComponent<ModelComponent>(floor);
        floorModel.textureScale = glm::vec2(uiinfo.floorScale);

        if (tick % 20 == 0) {
            auto block = addInstance(cubeMesh, rp3::BodyType::DYNAMIC, glm::vec3(0.5f), glm::vec3(0, 25, 0), glm::vec2(1.0f));
        }

        //for(int i=0; i<10; i++) {
        //    float fn = static_cast<float>(i) / 10.0f * 2.0f * 3.1415926f;
        //    float f = fn + time;
        //    auto& lightComp = ecsCoordinator.GetComponent<LightComponent>(lights[i]);
        //    lightComp.position = glm::vec4(20 * sin(f), 5, 20 * cos(f), 0);
        //    float s2 = sin(fn) * sin(fn);
        //    float c2 = cos(fn) * cos(fn);
        //    lightComp.color = glm::vec4(glm::vec3(20 * s2, 20 * c2, 20 * (1 - s2 - c2)), 0);
        //}
    }

    printf("Flushing GPU before shutdown...\n");
    vkDeviceWaitIdle(device.vkDevice);
    printf("Goodbye!\n");
}

void App::createECSSystems() {
    renderSystem = ecsCoordinator.RegisterSystem<RenderSystem>(device);
    physicsSystem = ecsCoordinator.RegisterSystem<PhysicsSystem>();
}

void App::createWorld() {
    auto terracottaDTex = renderSystem->createTextureFromFile("assets/textures/terracotta.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    auto terracottaNTex = renderSystem->createTextureFromFile("assets/textures/terracotta_normal.jpg", VK_FORMAT_R8G8B8A8_UNORM);
    cubeMesh = renderSystem->loadMesh("assets/models/cube.obj");
    auto lucyMesh = renderSystem->loadMesh("assets/models/lucy.obj");

    std::string diffuseTexFile;
    auto florianMesh = renderSystem->loadMesh("assets/models/florian_small.obj", &diffuseTexFile);
    auto florianTex = renderSystem->createTextureFromFile(diffuseTexFile, VK_FORMAT_R8G8B8A8_SRGB);
    auto florianTexSet = renderSystem->createTextureSet(florianTex);

    addInstance(cubeMesh, rp3::BodyType::DYNAMIC, glm::vec3(0.3f), glm::vec3(0, 0, 6), glm::vec2(1.0f, 1.0f));
    addInstance(cubeMesh, rp3::BodyType::DYNAMIC, glm::vec3(0.3f), glm::vec3(0.2, 1, 6.1), glm::vec2(1.0f, 1.0f));
    auto lucy = addInstance(lucyMesh, rp3::BodyType::DYNAMIC, glm::vec3(0.1f), glm::vec3(0.2, 3, 6.01), glm::vec2(1.0f));
    auto florian = addInstance(florianMesh, rp3::BodyType::DYNAMIC, glm::vec3(0.1f), glm::vec3(2.2, 3, 6.01), glm::vec2(1.0f), florianTexSet);
    physicsSystem->setMass(florian, 10);
    floor = addInstance(cubeMesh, rp3::BodyType::KINEMATIC, glm::vec3(25.0f, 0.2f, 25.0f), glm::vec3(0, -4, 6), glm::vec2(8.0f), renderSystem->createTextureSet(terracottaDTex, terracottaNTex));
    physicsSystem->setMass(lucy, 10);

    //auto light = ecsCoordinator.CreateEntity();
    //ecsCoordinator.AddComponent<LightComponent>(light, LightComponent {
    //    .position = glm::vec4(0, 25, 0, 0),
    //    .color = glm::vec4(glm::vec3(80), 0),
    //});

   // lights.resize(10);
   // for(int i=0; i<10; i++) {
   //     lights[i] = ecsCoordinator.CreateEntity();
   //     ecsCoordinator.AddComponent(lights[i], LightComponent{});
   // }
    //physicsSystem->setAngularVelocity(floor, glm::vec3(0.0f,0, 0.1f));
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

    auto& light = ecsCoordinator.AddComponent<LightComponent>(entity, LightComponent {
        .color = glm::vec4(3, 3, 5, 0) * 10.0f,
    });

    physicsSystem->addPositionListener(entity, (glm::vec3*)&light.position);
    return entity;
}