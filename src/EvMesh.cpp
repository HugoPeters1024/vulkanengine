#include "EvMesh.h"

EvMesh::EvMesh(EvDevice &device, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices, BoundingBox bb)
        : device(device), boundingBox(bb) {
    createVertexBuffer(vertices);
    createIndexBuffer(indices);
}

EvMesh::~EvMesh() {
    // Descriptor sets are destroyed when the pool is destroyed
    printf("Destroying index buffer\n");
    vmaDestroyBuffer(device.vmaAllocator, vkIndexBuffer, vkIndexMemory);

    printf("Destroying vertex buffer\n");
    vmaDestroyBuffer(device.vmaAllocator, vkVertexBuffer, vkVertexMemory);
}

void EvMesh::loadMesh(const std::string &filename, std::vector<Vertex> *vertices, std::vector<uint32_t> *indices, BoundingBox *box, std::string *diffuseTextureFile, std::string *normalTextureFile) {
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

    for(const auto& mat : materials) {
        if (diffuseTextureFile) *diffuseTextureFile = "./assets/textures/" + mat.diffuse_texname;
        if (normalTextureFile) *normalTextureFile = "./assets/textures/" + mat.normal_texname;
    }

    uint indexHead = 0;
    std::unordered_map<Vertex, uint32_t> indexMap(50000);

    for(const auto& shape : shapes) {
        for(size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            assert(shape.mesh.num_face_vertices[f] == 3 && "Plz only triangles for now");

            Vertex triangle[3];
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

                glm::vec3 normal(0.0f, 0.0f, 1.0f);

                if (idx.normal_index != -1) {
                    normal[0] = attrib.normals[3 * idx.normal_index + 0];
                    normal[1] = attrib.normals[3 * idx.normal_index + 1];
                    normal[2] = attrib.normals[3 * idx.normal_index + 2];
                }

                glm::vec3 tangent = glm::make_any_perp(normal);
                triangle[v] = Vertex { vpos, uv, normal, tangent };
            }

            // calculate the tangent if possible
            {
                const auto edge1 = triangle[1].position - triangle[0].position;
                const auto edge2 = triangle[2].position - triangle[0].position;
                const auto deltaUV1 = triangle[1].uv - triangle[0].uv;
                const auto deltaUV2 = triangle[2].uv - triangle[0].uv;

                const float invr = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
                if (invr > 0.00001f) {
                    const float r = 1.0f / invr;
                    const auto tangent = r * (edge1 * deltaUV2.y - edge2 * deltaUV1.y);

                    triangle[0].tangent = tangent;
                    triangle[1].tangent = tangent;
                    triangle[2].tangent = tangent;
                }
            }

            // insert triangle in the buffers
            for(const auto& vertex : triangle) {
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

void EvMesh::createVertexBuffer(const std::vector<Vertex> &vertices) {
    vertexCount = static_cast<uint32_t>(vertices.size());
//    assert(vertexCount > 0 && vertexCount % 3 == 0 && "vertexCount not a multiple of 3");

    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    device.createDeviceBuffer(bufferSize, (void*)vertices.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &vkVertexBuffer, &vkVertexMemory);
}

void EvMesh::createIndexBuffer(const std::vector<uint32_t> &indices) {
    indicesCount = static_cast<uint32_t>(indices.size());
    assert(indicesCount > 0 && indicesCount % 3 == 0 && "index count not a multiple of 3");

    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    device.createDeviceBuffer(bufferSize, (void*)indices.data(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &vkIndexBuffer, &vkIndexMemory);
}

void EvMesh::bind(VkCommandBuffer commandBuffer) {
    VkBuffer buffers[] = {vkVertexBuffer};
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, vkIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
}

void EvMesh::draw(VkCommandBuffer commandBuffer) const {
    vkCmdDrawIndexed(commandBuffer, indicesCount, 1, 0, 0, 0);
}