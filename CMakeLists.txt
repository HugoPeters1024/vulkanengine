cmake_minimum_required(VERSION 3.19)
project(vulkanray)

set(CMAKE_CXX_STANDARD 20)
file(GLOB src CONFIGURE_DEPENDS "src/*.cpp")

macro(shader)
    add_custom_command(
            OUTPUT glsl_shaders_bin/${ARGV0}.spv
            DEPENDS glsl_shaders/${ARGV0}
            COMMAND /usr/bin/glslc
            ARGS ${CMAKE_SOURCE_DIR}/glsl_shaders/${ARGV0} -o ${CMAKE_SOURCE_DIR}/glsl_shaders_bin/${ARGV0}.spv
            COMMENT building shaders
            VERBATIM)
    SET(src ${src} glsl_shaders_bin/${ARGV0}.spv)
endmacro()

shader("shader.vert")
shader("shader.frag")

add_executable(vulkanray main.cpp ${src})
set_target_properties(vulkanray PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(vulkanray PUBLIC ${CMAKE_SOURCE_DIR}/include/)

add_subdirectory(${CMAKE_SOURCE_DIR}/ecs)
link_directories(${CMAKE_SOURCE_DIR}/ecs)
target_include_directories(vulkanray PUBLIC ${CMAKE_SOURCE_DIR}/ecs/include/)
target_link_libraries(vulkanray ecs)

find_package(glm REQUIRED)
message(STATUS "GLM included at ${GLM_INCLUDE_DIR}")

find_package(glfw3 3.3.2 REQUIRED)
target_link_libraries(vulkanray glfw)

find_package(Vulkan REQUIRED)
target_link_libraries(vulkanray Vulkan::Vulkan)
