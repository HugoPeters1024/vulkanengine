#pragma once

#include "types.h"
#include "ComponentArray.h"
#include <unordered_map>
#include <memory>
#include <typeindex>

class ComponentManager {
private:
    std::unordered_map<std::type_index, ComponentType> m_componentTypes{};
    std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> m_componentArray{};
    ComponentType m_nextComponentType{};

    template<typename T>
    std::shared_ptr<ComponentArray<T>> GetComponentArray() {
        std::type_index typeName = typeid(T);
        assert(m_componentTypes.find(typeName) != m_componentTypes.end() && "Component not registered before use.");
        return std::static_pointer_cast<ComponentArray<T>>(m_componentArray[typeName]);
    }
public:
    template<typename T>
    void RegisterComponent() {
        std::type_index typeName = typeid(T);
        assert(m_componentTypes.find(typeName) == m_componentTypes.end());

        m_componentTypes.insert({typeName, m_nextComponentType});
        m_componentArray.insert({typeName, std::make_shared<ComponentArray<T>>()});
        m_nextComponentType++;
    }

    template<typename T>
    ComponentType GetComponentType() {
        std::type_index typeName = typeid(T);
        assert(m_componentTypes.find(typeName) != m_componentTypes.end() && "Component not registered before use.");

        // Return this component's type - used for creating signatures
        return m_componentTypes[typeName];
    }

    template<typename T>
    void AddComponent(Entity entity, T component) {
        GetComponentArray<T>()->InsertData(entity, component);
    }

    template<typename T>
    void RemoveComponent(Entity entity) {
        GetComponentArray<T>()->RemoveData(entity);
    }

    template<typename T>
    T& GetComponent(Entity entity) {
        return GetComponentArray<T>()->GetData(entity);
    }

    template<typename T>
    T* GetComponentArrayData() {
        return GetComponentArray<T>()->GetRawPointer();
    }

    template<typename T>
    bool HasComponent(Entity entity) {
        return GetComponentArray<T>()->HasEntity(entity);
    }

    void EntityDestroyed(Entity entity) {
        for(auto const& pair : m_componentArray) {
            auto const& component = pair.second;
            component->EntityDestroyed(entity);
        }
    }
};


