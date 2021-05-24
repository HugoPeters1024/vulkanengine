#pragma once
#include <GLFW/glfw3.h>
#include <memory>
#include <functional>
#include "ecs/ecs.h"
#include "EvInputHelper.h"

struct InputComponent {
    std::function<void(const EvInputHelper&)> handler;
};

class InputSystem : public System {
private:
    std::unique_ptr<EvInputHelper> inputHelper;
public:
    InputSystem(GLFWwindow*& window);
    void Update();

    Signature GetSignature() const override;
};

