#pragma once

#include <set>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "utils.hpp"

class EvWindow : NoCopy {
private:
    static void onResizeCallback(GLFWwindow* glfwWindow, int width, int height);
public:
    int width, height;
    std::string name;
    GLFWwindow* glfwWindow;
    bool wasResized = false;

    EvWindow(int w, int h, std::string name);
    ~EvWindow();

    void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) const;
    void collectInstanceExtensions(std::set<const char*>& instanceExtensions) const;
    void getFramebufferSize(int* width, int* height) const;

    bool shouldClose() const;
    void processEvents() const;
    void waitForEvent() const;
};
