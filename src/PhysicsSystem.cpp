#include "PhysicsSystem.h"

PhysicsSystem::PhysicsSystem() {
    rp3::PhysicsWorld::WorldSettings settings;
    settings.gravity = rp3::Vector3(0, 9.81, 0);
    world = physicsCommon.createPhysicsWorld(settings);
}

PhysicsSystem::~PhysicsSystem() {
    physicsCommon.destroyPhysicsWorld(world);
}

Signature PhysicsSystem::GetSignature() const {
    Signature ret;
    ret.set(m_coordinator->GetComponentType<PhysicsComponent>());
    ret.set(m_coordinator->GetComponentType<TransformComponent>());
    return ret;
}

void PhysicsSystem::Update() {
    world->update(1.0f / 60.0f);

    for(const auto& entity : m_entities) {
        const auto& physicsComp = m_coordinator->GetComponent<PhysicsComponent>(entity);
        auto& transformComp = m_coordinator->GetComponent<TransformComponent>(entity);
        rp3::Transform transform = physicsComp.rigidBody->getTransform();
        transformComp.transform = glm::mat4(1.0f);
        transform.getOpenGLMatrix((float*)&transformComp.transform);
        int lol = 1;
    }
}

rp3::RigidBody *PhysicsSystem::cubeBody(glm::vec3 size) {
    rp3::Vector3 position(0.0, 0.0, 6.0);
    rp3::Quaternion orientation = rp3::Quaternion::identity();
    rp3::Transform transform(position, orientation);

    auto ret = world->createRigidBody(transform);
    ret->setType(rp3::BodyType::DYNAMIC);
    return ret;
}
