#pragma once

#include <vulkan/vulkan.h>
#include "ecs/ecs.h"

struct RenderComponent {
    EvModel* model;
};

class RenderSystem : public System
{
private:
    EcsCoordinator& coordinator;
public:
    RenderSystem(EcsCoordinator& coordinator) : coordinator(coordinator) {};
    void Render(VkCommandBuffer commandBuffer)
    {
        for (const auto& entity : m_entities) {
            auto& component = coordinator.GetComponent<RenderComponent>(entity);
            component.model->bind(commandBuffer);
            component.model->draw(commandBuffer);
        }
    }
};