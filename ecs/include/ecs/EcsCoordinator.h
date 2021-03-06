#pragma once

#include <memory>
#include "types.h"
#include "ComponentManager.h"
#include "EntityManager.h"
#include "SystemManager.h"

class EcsCoordinator {
private:
    std::unique_ptr<ComponentManager> m_componentManager;
    std::unique_ptr<EntityManager> m_entityManager;
    std::unique_ptr<SystemManager> m_systemManager;

public:
    EcsCoordinator() {
        m_componentManager = std::make_unique<ComponentManager>();
        m_entityManager = std::make_unique<EntityManager>();
        m_systemManager = std::make_unique<SystemManager>();
    }

    Entity CreateEntity() {
        return m_entityManager->createEntity();
    }

    void DestroyEntity(Entity entity) {
        m_systemManager->EntityDestroyed(entity);
        m_entityManager->DestroyEntity(entity);
        m_componentManager->EntityDestroyed(entity);
    }

    template<typename T>
    void RegisterComponent() {
        m_componentManager->RegisterComponent<T>();
    }

    template<typename T>
    T& AddComponent(Entity entity, T component) {
        m_componentManager->AddComponent<T>(entity, component);

        auto signature = m_entityManager->GetSignature(entity);
        signature.set(m_componentManager->GetComponentType<T>(), true);
        m_entityManager->SetSignature(entity, signature);
        m_systemManager->EntitySignatureChanged(entity, signature);
        return GetComponent<T>(entity);
    }

    template<typename T>
    void RemoveComponent(Entity entity) {
        m_componentManager->RemoveComponent<T>(entity);
        auto signature = m_entityManager->GetSignature(entity);
        signature.set(m_componentManager->GetComponentType<T>(), false);
        m_entityManager->SetSignature(entity, signature);
        m_systemManager->EntitySignatureChanged(entity, signature);
    }

    template<typename T>
    T& GetComponent(Entity entity) {
        return m_componentManager->GetComponent<T>(entity);
    }

    template<typename T>
    T* GetComponentArrayData() {
        return m_componentManager->GetComponentArrayData<T>();
    }

    template<typename T>
    bool HasComponent(Entity entity) {
        return m_componentManager->HasComponent<T>(entity);
    }

    template<typename T>
    ComponentType GetComponentType() {
        return m_componentManager->GetComponentType<T>();
    }

    template<typename T, typename... Args>
    std::shared_ptr<T> RegisterSystem(Args&&... args) {
        auto system = m_systemManager->RegisterSystem<T>(std::forward<Args>(args)...);
        // set reference to myself
        system->m_coordinator = this;

        // Run any sub registration
        system->RegisterStage();

        // now call upons the signature
        SetSystemSignature<T>(system->GetSignature());
        return system;
    }


    template<typename T>
    void SetSystemSignature(Signature signature) {
        m_systemManager->SetSignature<T>(signature);
    }
};

