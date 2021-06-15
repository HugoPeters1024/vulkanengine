#pragma once

#include "core.h"
#include "Primitives.h"
#include "Components.h"

namespace rp3 = reactphysics3d;


class PhysicsSystem : public System
{
    rp3::PhysicsCommon physicsCommon;
    rp3::PhysicsWorld* world;
public:
    PhysicsSystem();
    ~PhysicsSystem();
    Signature GetSignature() const override;

    void RegisterStage() override;

    void EntityDestroyed(Entity entity) override;


    void Update(float forceField);
    void linkModelComponent(Entity entity);
    void addIntersectionBoxBody(Entity entity, BoundingBox box);
    void addPositionListener(Entity, glm::vec3* dst);
    void setMass(Entity entity, float mass);
    void applyForce(Entity entity, glm::vec3 force);
    void setAngularVelocity(Entity entity, glm::vec3 eulerAngles);
    void setWorldGravity(float gravity);

    rp3::RigidBody *createRigidBody(rp3::BodyType bodyType, glm::vec3 pos);
};