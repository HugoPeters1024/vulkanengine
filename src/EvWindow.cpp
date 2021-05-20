#include <cstdio>
#include "EvWindow.h"

EvWindow::EvWindow(int w, int h, std::string name) : width(w), height(h), name(name) {
    if (!glfwInit()) {
        throw std::runtime_error("Could not initialize GLFW");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindow = glfwCreateWindow(w, h, name.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(glfwWindow, this);
    glfwSetWindowSizeCallback(glfwWindow, onResizeCallback);
}

EvWindow::~EvWindow() {
    printf("Destroying window\n");
    glfwDestroyWindow(glfwWindow);
    glfwTerminate();
}

void EvWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) const {
    vkCheck(glfwCreateWindowSurface(instance, glfwWindow, nullptr, surface));
}

void EvWindow::collectInstanceExtensions(std::set<const char *> &instanceExtensions) const {
    uint extCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extCount);
    for(uint i=0; i<extCount; i++) {
        instanceExtensions.insert(glfwExtensions[i]);
    }
}

bool EvWindow::shouldClose() const {
    return glfwWindowShouldClose(glfwWindow);
}

void EvWindow::processEvents() const {
    glfwPollEvents();
}

void EvWindow::waitForEvent() const {
    glfwWaitEvents();
}

void EvWindow::getFramebufferSize(int *width, int *height) const {
    *width = 0;
    *height = 0;
    do {
        glfwGetFramebufferSize(glfwWindow, width, height);
        glfwWaitEvents();
    } while (*width == 0 || *height == 0);
}

void EvWindow::onResizeCallback(GLFWwindow* glfwWindow, int width, int height) {
    auto window = reinterpret_cast<EvWindow*>(glfwGetWindowUserPointer(glfwWindow));
    window->width = width;
    window->height = height;
    window->wasResized = true;
    printf("window was resized to (%i, %i)\n", width, height);
}



