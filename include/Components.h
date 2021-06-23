#pragma once

#include "core.h"
#include "EvMesh.h"

struct PhysicsComponent {
    rp3d::RigidBody* rigidBody{};
    glm::mat4* transformResult = nullptr;
    std::vector<glm::vec3*> positionListeners;

    void linkMat4(glm::mat4* t) { transformResult = t; }
};

struct TextureSet : NoCopy {
    std::vector<VkDescriptorSet> descriptorSets;
};

struct ModelComponent {
    EvMesh* mesh = nullptr;
    TextureSet* textureSet = nullptr;
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    glm::vec2 textureScale{1.0f, 1.0f};
    glm::mat4 transform{1.0f};
};

struct LightComponent {
    glm::vec4 position;
    glm::vec4 color;
};
