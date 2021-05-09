#pragma once

#include <set>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "utils.h"

class EvWindow : NoCopy {
public:
    int width, height;
    std::string name;
    GLFWwindow* glfwWindow;

    EvWindow(int w, int h, std::string name);
    ~EvWindow();

    void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) const;
    void collectInstanceExtensions(std::set<const char*>& instanceExtensions) const;
    void getFramebufferSize(int* width, int* height) const;

    bool shouldClose() const;
    void processEvents() const;
};
