cmake_minimum_required(VERSION 3.16)
project(LowRenderer)

set(VULKAN_SDK_DIR $ENV{VULKAN_SDK})

find_package(Vulkan REQUIRED)
find_library(SHADERC_COMBINED_DEBUG shaderc_combinedd.lib PATHS ${VULKAN_SDK_DIR}/Lib)
find_library(SHADERC_COMBINED shaderc_combined.lib PATHS ${VULKAN_SDK_DIR}/Lib)

add_subdirectory(Low)
add_subdirectory(Lower)