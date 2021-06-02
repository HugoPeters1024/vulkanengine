#define TINYOBJLOADER_IMPLEMENTATION
#include "EvModel.h"

EvModel::EvModel(EvDevice &device, const std::string &objFile, const EvTexture *texture)
        : device(device), texture(texture) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    loadModel(objFile, &vertices, &indices, &boundingBox);
    createVertexBuffer(vertices);
    createIndexBuffer(indices);
}

EvModel::~EvModel() {
    // Descriptor sets are destroyed when the pool is destroyed
    printf("Destroying index buffer\n");
    vmaDestroyBuffer(device.vmaAllocator, vkIndexBuffer, vkIndexMemory);

    printf("Destroying vertex buffer\n");
    vmaDestroyBuffer(device.vmaAllocator, vkVertexBuffer, vkVertexMemory);
}

void EvModel::loadModel(const std::string &filename, std::vector<Vertex> *vertices, std::vector<uint32_t> *indices, BoundingBox *box) {
    *box = BoundingBox::InsideOut();
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

    uint indexHead = 0;
    std::unordered_map<Vertex, uint32_t> indexMap;

    for(const auto& shape : shapes) {
        for(size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            assert(shape.mesh.num_face_vertices[f] == 3 && "Plz only triangles for now");
            for(size_t v = 0; v<3; v++) {
                const auto& idx = shape.mesh.indices[f * 3 + v];

                glm::vec3 vpos;
                vpos[0] = attrib.vertices[3 * idx.vertex_index + 0];
                vpos[1] = attrib.vertices[3 * idx.vertex_index + 1];
                vpos[2] = attrib.vertices[3 * idx.vertex_index + 2];

                box->consumePoint(vpos);

                glm::vec2 uv(0.0f);
                if (idx.texcoord_index != -1) {
                    uv[0] = attrib.texcoords[2 * idx.texcoord_index + 0];
                    uv[1] = attrib.texcoords[2 * idx.texcoord_index + 1];
                }

                glm::vec3 normal(0.0f);

                if (idx.normal_index != -1) {
                    normal[0] = attrib.normals[3 * idx.normal_index + 0];
                    normal[1] = attrib.normals[3 * idx.normal_index + 1];
                    normal[2] = attrib.normals[3 * idx.normal_index + 2];
                }

                Vertex vertex { vpos, uv, normal };
                if (indexMap.find(vertex) == indexMap.end()) {
                    indexMap.insert({vertex, indexHead++});
                }

                indices->push_back(indexMap[vertex]);
            }
        }
    }

    vertices->resize(indexHead);
    for(auto [vertex, index] : indexMap) {
        (*vertices)[index] = vertex;
    }


    printf("Loaded model %s: Vertices: %lu, Indices: %lu\n", filename.c_str(), vertices->size(), indices->size());
}

void EvModel::createVertexBuffer(const std::vector<Vertex> &vertices) {
    vertexCount = static_cast<uint32_t>(vertices.size());
//    assert(vertexCount > 0 && vertexCount % 3 == 0 && "vertexCount not a multiple of 3");

    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    device.createDeviceBuffer(bufferSize, (void*)vertices.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &vkVertexBuffer, &vkVertexMemory);
}

void EvModel::createIndexBuffer(const std::vector<uint32_t> &indices) {
    indicesCount = static_cast<uint32_t>(indices.size());
    assert(indicesCount > 0 && indicesCount % 3 == 0 && "index count not a multiple of 3");

    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    device.createDeviceBuffer(bufferSize, (void*)indices.data(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &vkIndexBuffer, &vkIndexMemory);
}

void EvModel::bind(VkCommandBuffer commandBuffer) {
    assert(texture && "Texture must be set");
    assert(!vkDescriptorSets.empty() && "Descriptor set must have been created");
    VkBuffer buffers[] = {vkVertexBuffer};
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, vkIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
}

void EvModel::draw(VkCommandBuffer commandBuffer) const {
    vkCmdDrawIndexed(commandBuffer, indicesCount, 1, 0, 0, 0);
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
    return {
        VkVertexInputAttributeDescription {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, position),
        },
        VkVertexInputAttributeDescription {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, uv),
        },
        VkVertexInputAttributeDescription {
                .location = 2,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, normal),
        },
    };
}
