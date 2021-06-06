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
    return ret;
}

void PhysicsSystem::Update() {
    world->update(1.0f / 60.0f);

    for(const auto& entity : m_entities) {
        auto& physicsComp = m_coordinator->GetComponent<PhysicsComponent>(entity);

        if (physicsComp.transformResult != nullptr) {
            rp3::Transform transform = physicsComp.rigidBody->getTransform();
            transform.getOpenGLMatrix((float*)physicsComp.transformResult);
        }
    }
}

void PhysicsSystem::linkModelComponent(Entity entity) {
    assert(m_coordinator->HasComponent<PhysicsComponent>(entity) && "entity must have physics component");
    assert(m_coordinator->HasComponent<ModelComponent>(entity) && "entity must have model component");
    auto& physics = m_coordinator->GetComponent<PhysicsComponent>(entity);
    auto& model = m_coordinator->GetComponent<ModelComponent>(entity);
    physics.linkMat4(&model.transform);
}

rp3::RigidBody* PhysicsSystem::createRigidBody(rp3::BodyType bodyType, glm::vec3 pos) {
    rp3::Vector3 position(pos.x, pos.y, pos.z);
    rp3::Quaternion orientation = rp3::Quaternion::identity();
    rp3::Transform transform(position, orientation);

    auto ret = world->createRigidBody(transform);
    ret->setType(bodyType);
    return ret;
}

void PhysicsSystem::addIntersectionBoxBody(Entity entity, BoundingBox box) {
    assert(m_coordinator->HasComponent<PhysicsComponent>(entity));
    auto halfExtents = (box.vmax - box.vmin) / 2.0f;
    auto shape = physicsCommon.createBoxShape(rp3::mkVector3(halfExtents));
    auto transform = rp3::Transform::identity();
    auto& physics = m_coordinator->GetComponent<PhysicsComponent>(entity);
    physics.rigidBody->addCollider(shape, transform);
}

void PhysicsSystem::setMass(Entity entity, float mass) {
    assert(m_coordinator->HasComponent<PhysicsComponent>(entity));
    auto& physics = m_coordinator->GetComponent<PhysicsComponent>(entity);
    physics.rigidBody->setMass(mass);
}

void PhysicsSystem::setAngularVelocity(Entity entity, glm::vec3 eulerAngles) {
    assert(m_coordinator->HasComponent<PhysicsComponent>(entity));
    auto& physics = m_coordinator->GetComponent<PhysicsComponent>(entity);
    physics.rigidBody->setAngularVelocity(rp3::mkVector3(eulerAngles));
}

void PhysicsSystem::setWorldGravity(float gravity) {
    world->setGravity(rp3::Vector3(0, gravity, 0));
}
