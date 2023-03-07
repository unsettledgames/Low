cmake_minimum_required(VERSION 3.16)
project(Lower)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE LOWER_SRC
	"src/*.h",
	"src/*.cpp"
)

set(LOWER_EXT
	"vendor/imguizmo/ImGuizmo.cpp"
)

add_executable(Lower ${LOWER_SRC} ${LOWER_EXT})

add_subdirectory(vendor/imgui)
add_subdirectory(vendor/imguizmo)

target_link_libraries(Lower
	PUBLIC Low
)

target_include_directories(Lower
	PUBLIC src
	PUBLIC vendor/imguizmo
	PUBLIC vendor/imgui
)

# Keep the project structure
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${LOWER_SRC} ${LOWER_EXT})