project(imgui CXX)
add_library(imgui
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui.cpp"

        "${CMAKE_CURRENT_SOURCE_DIR}/imgui_demo.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui_draw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui_widgets.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui_tables.cpp"

        "${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_glfw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_vulkan.cpp"
        )
set_target_properties(imgui PROPERTIES LINKER_LANGUAGE CXX)
