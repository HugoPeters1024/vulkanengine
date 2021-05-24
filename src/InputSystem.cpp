#include "InputSystem.h"

InputSystem::InputSystem(GLFWwindow *&window) {
    inputHelper = std::make_unique<EvInputHelper>(window);
}

Signature InputSystem::GetSignature() const {
    auto ret = Signature();
    ret.set(m_coordinator->GetComponentType<InputComponent>());
    return ret;
}

void InputSystem::Update() {
    inputHelper->swapBuffers();

    for(const auto& entity : m_entities) {
        const auto& comp = m_coordinator->GetComponent<InputComponent>(entity);
        comp.handler(*inputHelper);
    }
}
