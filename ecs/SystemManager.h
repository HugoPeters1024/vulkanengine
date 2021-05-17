#pragma once

#include <memory>
#include <unordered_map>
#include <cassert>
#include "types.h"
#include "System.h"

class SystemManager {
private:
    std::unordered_map<const char*, Signature> m_signatures{};
    std::unordered_map<const char*, std::shared_ptr<System>> m_systems{};
public:
    template<typename T, typename... Args>
    std::shared_ptr<T> RegisterSystem(Args&&... args) {
        const char* typeName = typeid(T).name();

        assert(m_systems.find(typeName) == m_systems.end() && "Registering system more than once.");

        // Create a pointer to the system and return it so it can be used externally
        auto system = std::make_shared<T>(std::forward<Args>(args)...);
        m_systems.insert({typeName, system});
        return system;
    }

    template<typename T>
    void SetSignature(Signature signature) {
        const char* typeName = typeid(T).name();
        assert(m_systems.find(typeName) != m_systems.end() && "System used before register");
        m_signatures.insert({typeName, signature});
    }

    void EntityDestroyed(Entity entity) {
        for (auto const& pair : m_systems) {
            auto const& system = pair.second;
            system->m_entities.erase(entity);
        }
    }

    void EntitySignatureChanged(Entity entity, Signature signature) {
        for (auto const& pair : m_systems) {
            auto const& type = pair.first;
            auto const& system = pair.second;
            auto const& systemSignature = m_signatures[type];

            if ((signature & systemSignature) == systemSignature) {
                system->m_entities.insert((entity));
            } else {
                system->m_entities.erase(entity);
            }
        }
    }
};

