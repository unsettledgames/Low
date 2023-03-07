cmake_minimum_required(VERSION 3.16)
project(Lower)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE LOWER_SRC
	"src/*.h",
	"src/*.cpp"
)

add_executable(Lower ${LOWER_SRC})

target_link_libraries(Lower
	PUBLIC Low
)

