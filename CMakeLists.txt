cmake_minimum_required(VERSION 3.19)
project(vulkanray)

set(CMAKE_CXX_STANDARD 20)
file(GLOB_RECURSE src CONFIGURE_DEPENDS "include/*.h" "include/*.hpp" "src/*.cpp")

macro(shader)
    add_custom_command(
            OUTPUT assets/shaders_bin/${ARGV0}.spv
            DEPENDS assets/shaders/${ARGV0}
            COMMAND /usr/bin/glslc
            ARGS ${CMAKE_SOURCE_DIR}/assets/shaders/${ARGV0} -o ${CMAKE_SOURCE_DIR}/assets/shaders_bin/${ARGV0}.spv -O
            COMMENT building shaders
            VERBATIM)
    SET(src ${src} assets/shaders_bin/${ARGV0}.spv)
endmacro()

shader("screenfill.vert")

shader("forward.vert")
shader("forward.frag")

shader("skybox.vert")
shader("skybox.frag")

shader("post.frag")

shader("depth.vert")

shader("blur.comp")

add_executable(vulkanray main.cpp ${src})
set_target_properties(vulkanray PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(vulkanray PUBLIC ${CMAKE_SOURCE_DIR}/include/)
target_include_directories(vulkanray PUBLIC ${CMAKE_SOURCE_DIR}/external)
target_precompile_headers(vulkanray PRIVATE include/core.h)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/vks)
link_directories(${CMAKE_SOURCE_DIR}/external/vks)
target_include_directories(vulkanray PUBLIC ${CMAKE_SOURCE_DIR}/vks/)
target_link_libraries(vulkanray vks)

add_subdirectory(${CMAKE_SOURCE_DIR}/ecs)
link_directories(${CMAKE_SOURCE_DIR}/ecs)
target_include_directories(vulkanray PUBLIC ${CMAKE_SOURCE_DIR}/ecs/include/)
target_link_libraries(vulkanray ecs)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/reactphysics3d)
link_directories(${CMAKE_SOURCE_DIR}/external/reactphysics3d)
target_include_directories(vulkanray PUBLIC ${CMAKE_SOURCE_DIR}/external/reactphysics3d/include/)
target_link_libraries(vulkanray reactphysics3d)


add_subdirectory(${CMAKE_SOURCE_DIR}/external/glfw)
link_directories(${CMAKE_SOURCE_DIR}/external/glfw)
target_include_directories(vulkanray PUBLIC ${CMAKE_SOURCE_DIR}/external/glfw/include)
target_link_libraries(vulkanray glfw)


add_subdirectory(${CMAKE_SOURCE_DIR}/external/imgui)
link_directories(${CMAKE_SOURCE_DIR}/external/imgui)
target_include_directories(vulkanray PRIVATE ${CMAKE_SOURCE_DIR}/external/imgui)
target_link_libraries(vulkanray imgui)

find_package(glm REQUIRED)
message(STATUS "GLM included at ${GLM_INCLUDE_DIR}")

find_package(Vulkan REQUIRED)
target_link_libraries(vulkanray Vulkan::Vulkan)
