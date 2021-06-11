#pragma once

#include "core.h"

class NoCopy
{
public:
    NoCopy(NoCopy const&) = delete;
    NoCopy& operator=(NoCopy const&) = delete;
    NoCopy() = default;
};

#define vkCheck(x)                                                                     \
    do                                                                                  \
    {                                                                                   \
        VkResult err = x;                                                               \
        if (err)                                                                        \
        {                                                                               \
            printf("Detected Vulkan error %d at %s:%d.\n", int(err), __FILE__, __LINE__); \
            abort();                                                                    \
        }                                                                               \
    } while (0)


inline bool checkValidationLayersSupported(const std::vector<const char*>& layers) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    VkLayerProperties availableLayers[layerCount];
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    for (const char* layerName : layers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

inline std::vector<char> readFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filepath);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), static_cast<long>(fileSize));
    file.close();
    return buffer;
}

struct pair_hash {
    template<class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2> &p) const {
        auto h1 = std::hash<T1>()(p.first);
        auto h2 = std::hash<T1>()(p.second);
        return h1 ^ h2;
    }
};

namespace reactphysics3d {
    inline Vector3 mkVector3(glm::vec3 v) {
        return Vector3(v.x, v.y, v.z);
    }
};

namespace glm {
    inline vec3 sphericalToCartesian(float theta, float phi) {
        return vec3(
                cos(theta) * sin(phi),
                cos(phi),
                sin(theta) * sin(phi)
        );
    }
}