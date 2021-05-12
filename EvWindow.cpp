#include <cstdio>
#include "EvWindow.h"

EvWindow::EvWindow(int w, int h, std::string name) : width(w), height(h), name(name) {
    if (!glfwInit()) {
        throw std::runtime_error("Could not initialize GLFW");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindow = glfwCreateWindow(w, h, name.c_str(), nullptr, nullptr);
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

void EvWindow::getFramebufferSize(int *width, int *height) const {
    glfwGetFramebufferSize(glfwWindow, width, height);
}


