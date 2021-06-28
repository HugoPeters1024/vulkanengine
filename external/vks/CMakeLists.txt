project(vks CXX)
add_library(vks
        "${CMAKE_CURRENT_SOURCE_DIR}/VulkanInitializers.hpp"

        "${CMAKE_CURRENT_SOURCE_DIR}/VulkanTools.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/VulkanTools.cpp"
        )
set_target_properties(vks PROPERTIES LINKER_LANGUAGE CXX)
