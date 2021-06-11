#pragma once

#include "RenderSystem.h"
#include "Primitives.h"

namespace rp3 = reactphysics3d;

struct PhysicsComponent {
    rp3::RigidBody* rigidBody{};
    glm::mat4* transformResult = nullptr;

    void linkMat4(glm::mat4* t) { transformResult = t; }
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
    void linkModelComponent(Entity entity);
    void addIntersectionBoxBody(Entity entity, BoundingBox box);
    void setMass(Entity entity, float mass);
    void setAngularVelocity(Entity entity, glm::vec3 eulerAngles);
    void setWorldGravity(float gravity);

    rp3::RigidBody *createRigidBody(rp3::BodyType bodyType, glm::vec3 pos);
};