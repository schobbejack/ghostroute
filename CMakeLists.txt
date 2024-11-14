cmake_minimum_required(VERSION 3.25)

project(ghostroute)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SRC "${ROOT}/src/*.cpp")
file(GLOB_RECURSE INC "${ROOT}/include/*.hpp")

add_executable(ghostroute
	${SRC}
	${INC}
)

target_include_directories(ghostroute
	PUBLIC
		$<BUILD_INTERFACE:${ROOT}/include>
		$<INSTALL_INTERFACE:include>
)
