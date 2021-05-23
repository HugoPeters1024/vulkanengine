#include "EvModel.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

EvModel::EvModel(EvDevice &device, const std::vector<Vertex> &vertices) : device(device) {
    createVertexBuffers(vertices);
}

EvModel::EvModel(EvDevice &device, const std::string &objFile)
        : EvModel(device, loadModel(objFile)) {
}

EvModel::~EvModel() {
    printf("Destroying vertex buffer\n");
    vmaDestroyBuffer(device.vmaAllocator, vkVertexBuffer, vkVertexMemory);
}

std::vector<Vertex> EvModel::loadModel(const std::string& filename) {
    tinyobj:: ObjReaderConfig config;

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(filename, config)) {
        throw std::runtime_error("Tiny obj error: " + reader.Error());
    }

    if (!reader.Warning().empty()) {
        printf("Tiny obj warning: %s", reader.Warning().c_str());
    }

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();
    const auto& materials = reader.GetMaterials();

    std::vector<Vertex> ret;

    for(const auto& shape : shapes) {
        for(size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            assert(shape.mesh.num_face_vertices[f] == 3 && "Plz only triangles for now");
            for(size_t v = 0; v<3; v++) {
                const auto& idx = shape.mesh.indices[f * 3 + v];
                float vx = attrib.vertices[3 * idx.vertex_index + 0];
                float vy = attrib.vertices[3 * idx.vertex_index + 1];
                float vz = attrib.vertices[3 * idx.vertex_index + 2];
                ret.push_back(Vertex { glm::vec3(vx, vy, vz) });
            }
        }
    }

    return ret;
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

void EvModel::draw(VkCommandBuffer commandBuffer) const {
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
