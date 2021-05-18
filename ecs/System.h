#pragma once

#include <set>
#include "types.h"

class EcsCoordinator;

class System {
public:
    std::set<Entity> m_entities;
    EcsCoordinator* m_coordinator;

    virtual Signature GetSignature() const = 0;
};
