#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "include/App.h"

int main() {
    App app{};
    app.Run();
    return 0;
}
