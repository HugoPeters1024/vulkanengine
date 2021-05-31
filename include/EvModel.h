#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <cstring>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <tiny_obj_loader.h>
#include "EvDevice.h"
#include "EvTexture.h"


struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;

    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
};

class EvModel : NoCopy {
private:
    EvDevice& device;
    VkBuffer vkVertexBuffer;
    VmaAllocation vkVertexMemory;
    uint32_t vertexCount;

    void createVertexBuffers(const std::vector<Vertex>& vertices);
    static std::vector<Vertex> loadModel(const std::string& filename);

public:
    const EvTexture *texture = nullptr;
    std::vector<VkDescriptorSet> vkDescriptorSets;
    EvModel(EvDevice& device, const std::vector<Vertex>& vertices, const EvTexture* texture);
    EvModel(EvDevice &device, const std::string &objFile, const EvTexture *texture);
    ~EvModel();

    void bind(VkCommandBuffer commandBuffer);
    void draw(VkCommandBuffer commandBuffer) const;
};