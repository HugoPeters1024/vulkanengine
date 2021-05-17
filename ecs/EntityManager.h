#pragma once

#include "types.h"
#include <queue>
#include <array>
#include <cassert>

class EntityManager {
private:
    std::queue<Entity> m_availableEntities{};
    std::array<Signature, MAX_ENTITIES> m_signatures{};
    uint32_t m_livingEntityCount{};

public:
    EntityManager() {
        // Initialize the queue with all available entity IDs
        for(Entity entity = 0; entity < MAX_ENTITIES; ++entity) {
            m_availableEntities.push(entity);
        }
    }

    Entity createEntity() {
        assert(m_livingEntityCount < MAX_ENTITIES && "Ran out of entities!");

        Entity id = m_availableEntities.front();
        m_availableEntities.pop();
        m_livingEntityCount++;
        return id;
    }

    void DestroyEntity(Entity entity) {
        assert(entity < MAX_ENTITIES && "Entity out of range");
        m_signatures[entity].reset();
        m_availableEntities.push(entity);
        m_livingEntityCount--;
    }

    void SetSignature(Entity entity, Signature signature) {
        assert(entity < MAX_ENTITIES && "Entity out of range");
        m_signatures[entity] = signature;
    }

    Signature GetSignature(Entity entity) {
        assert(entity < MAX_ENTITIES && "Entity out of range");
        return m_signatures[entity];
    }
};
