#pragma once


struct BoundingBox {
    glm::vec3 vmin, vmax;

    void consumePoint(glm::vec3 p) {
        vmin = glm::min(vmin, p);
        vmax = glm::max(vmax, p);
    }
    static BoundingBox InsideOut() {
        return {
            .vmin = glm::vec3(std::numeric_limits<float>::infinity()),
            .vmax = glm::vec3(-std::numeric_limits<float>::infinity()),
        };
    }

    BoundingBox operator* (const glm::vec3& s) const {
        return BoundingBox {
            .vmin = vmin * s,
            .vmax = vmax * s,
        };
    }
};

struct Vertex {
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 normal;

    bool operator ==(const Vertex& o) const {
        return position == o.position && uv == o.uv && normal == o.normal;
    }

    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0] = VkVertexInputBindingDescription {
                .binding = 0,
                .stride = sizeof(Vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };
        return bindingDescriptions;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
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
};