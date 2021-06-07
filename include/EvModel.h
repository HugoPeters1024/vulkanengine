#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <cstring>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <tiny_obj_loader.h>
#include "EvDevice.h"
#include "EvTexture.h"
#include "Primitives.h"



namespace std {
    template<>
    struct hash<Vertex> {
        std::size_t operator() (const Vertex& vertex) const {
            auto phash = std::hash<glm::vec3>()(vertex.position);
            auto uvhash = std::hash<glm::vec2>()(vertex.uv);
            auto nhash = std::hash<glm::vec3>()(vertex.normal);
            return phash ^ uvhash ^ nhash;
        }
    };
}

class EvModel : NoCopy {
private:
    EvDevice& device;
    VkBuffer vkVertexBuffer;
    VmaAllocation vkVertexMemory;
    uint32_t vertexCount;
    VkBuffer vkIndexBuffer;
    VmaAllocation vkIndexMemory;
    uint32_t indicesCount;

    void createVertexBuffer(const std::vector<Vertex>& vertices);
    void createIndexBuffer(const std::vector<uint32_t>& indices);
    static void loadModel(const std::string &filename, std::vector<Vertex> *vertices, std::vector<uint32_t> *indices, BoundingBox *box);

public:
    const EvTexture *texture = nullptr;
    std::vector<VkDescriptorSet> vkDescriptorSets;
    BoundingBox boundingBox;
    EvModel(EvDevice &device, const std::string &objFile, const EvTexture *texture);
    ~EvModel();

    void bind(VkCommandBuffer commandBuffer);
    void draw(VkCommandBuffer commandBuffer) const;
};