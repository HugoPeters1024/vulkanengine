#include "EvWindow.h"
#include "EvDevice.h"
#include "EvSwapchain.h"

#pragma once

class App {
    EvWindow window{640, 480, "My app"};
    EvDeviceInfo deviceInfo{};
    EvDevice device{deviceInfo, window};
    EvSwapchain swapchain{device};

public:
    void Run();
};
