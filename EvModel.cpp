#include "EvModel.h"

EvModel::EvModel(EvDevice &device, const std::vector<Vertex> &vertices) : device(device) {
    createVertexBuffers(vertices);
}

EvModel::~EvModel() {
    printf("Destroying vertex buffer\n");
    vmaDestroyBuffer(device.vmaAllocator, vkVertexBuffer, vkVertexMemory);
}

void EvModel::createVertexBuffers(const std::vector<Vertex> &vertices) {
    vertexCount = static_cast<uint32_t>(vertices.size());
    assert(vertexCount > 0 && vertexCount % 3 == 0 && "vertexCount not a multiple of 3");

    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    device.createDeviceBuffer(bufferSize, (void*)vertices.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &vkVertexBuffer, &vkVertexMemory);
}

void EvModel::bind(VkCommandBuffer commandBuffer) {
    VkBuffer buffers[] = {vkVertexBuffer};
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
}

void EvModel::draw(VkCommandBuffer commandBuffer) {
    vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
}

std::vector<VkVertexInputBindingDescription> Vertex::getBindingDescriptions() {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0] = VkVertexInputBindingDescription {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
    return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> Vertex::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(1);
    attributeDescriptions[0] = VkVertexInputAttributeDescription {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = 0,
    };
    return attributeDescriptions;
}
