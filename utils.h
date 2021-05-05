#pragma once
#include <cstdlib>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <cstring>

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