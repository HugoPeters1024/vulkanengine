#include "App.h"

void App::Run() {
    while (!window.shouldClose()) {
        window.processEvents();
    }
}
