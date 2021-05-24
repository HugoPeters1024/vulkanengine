#pragma once
#include <GLFW/glfw3.h>
#include <vector>

class EvInputHelper {
private:
    GLFWwindow*& window;
    std::vector<int> key_state, old_key_state;

public:
    EvInputHelper(GLFWwindow*& window);
    void swapBuffers();

    bool isDown(int glfwKey) const;
    bool isPressed(int glfwKey) const;
};