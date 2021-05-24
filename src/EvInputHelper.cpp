#include "EvInputHelper.h"

EvInputHelper::EvInputHelper(GLFWwindow *&window) : window(window) {
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);

    uint key_size = GLFW_KEY_LAST + 1;
    key_state = std::vector<int>(key_size);
    old_key_state = std::vector<int>(key_size);
}

void EvInputHelper::swapBuffers() {
   // Keys below barcode 32 are not in use and generate errors
   for(int i=32; i<key_state.size(); i++) {
       old_key_state[i] = key_state[i];
       key_state[i] = glfwGetKey(window, i);
   }
}

bool EvInputHelper::isDown(int glfwKey) const {
    return key_state[glfwKey] == GLFW_PRESS;
}

bool EvInputHelper::isPressed(int glfwKey) const {
    return key_state[glfwKey] == GLFW_PRESS && old_key_state[glfwKey] != GLFW_PRESS;
}
