#include "EvModel.h"

EvModel::EvModel(EvDevice &device, const std::vector<Vertex> &vertices) : device(device) {
    createVertexBuffers(vertices);
}

EvModel::~EvModel() {
    printf("Destroying vertex buffer\n");
    vkDestroyBuffer(device.vkDevice, vkVertexBuffer, nullptr);
    printf("Free vertex buffer memory");
    vkFreeMemory(device.vkDevice, vkVertexMemory, nullptr);
}

void EvModel::createVertexBuffers(const std::vector<Vertex> &vertices) {
    vertexCount = static_cast<uint32_t>(vertices.size());
    assert(vertexCount > 0 && vertexCount % 3 == 0 && "vertexCount not a multiple of 3");

    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    device.createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vkVertexBuffer, &vkVertexMemory);

    void* data;
    vkCheck(vkMapMemory(device.vkDevice, vkVertexMemory, 0, bufferSize, 0, &data));
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device.vkDevice, vkVertexMemory);
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
