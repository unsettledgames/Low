cmake_minimum_required(VERSION 3.16)
project(Low)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE LOW_SRC
	"src/*.h",
	"src/*.cpp",
	"vendor/stb/stb_image.h",
	"vendor/tiny_obj/tiny_obj_loader.h",
	"vendor/stb/stb_impl.cpp"
)

link_directories(${VULKAN_SDK_DIR}/Lib)
add_library(Low STATIC ${LOW_SRC})

add_subdirectory(vendor/glm)
add_subdirectory(vendor/GLFW)


target_precompile_headers(Low
	PRIVATE src/Lowpch.h
)

target_include_directories(Low 
	PUBLIC src
	PUBLIC ${Vulkan_INCLUDE_DIRS}
	PUBLIC vendor/glm/glm
	PUBLIC vendor/GLFW/include
	PUBLIC vendor/imgui
	PUBLIC ${VULKAN_SDK_DIR}/Include
	PUBLIC vendor/stb
	PUBLIC vendor/tiny_obj
)


target_link_libraries(Low
	PUBLIC ${Vulkan_LIBRARIES}
	PUBLIC glm
	PUBLIC glfw
	PUBLIC imgui
	debug ${SHADERC_COMBINED_DEBUG}
	optimized ${SHADERC_COMBINED}
)

# Keep the project structure
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${LOW_SRC})
