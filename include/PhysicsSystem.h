#pragma once

#include <ecs/ecs.h>
#include <reactphysics3d/reactphysics3d.h>
#include "RenderSystem.h"

namespace rp3 = reactphysics3d;

struct PhysicsComponent {
    rp3::RigidBody* rigidBody;
};

class PhysicsSystem : public System
{
    rp3::PhysicsCommon physicsCommon;
    rp3::PhysicsWorld* world;
public:
    PhysicsSystem();
    ~PhysicsSystem();
    Signature GetSignature() const override;

    void Update();
    rp3::RigidBody* cubeBody(glm::vec3 size);
};