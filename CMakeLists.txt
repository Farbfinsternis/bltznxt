cmake_minimum_required(VERSION 3.10)
project(BlitzNext)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Third-party dependencies
include_directories(src/thirdparty)
include_directories(src/compiler)

if(JSON_INCLUDE_DIR)
    include_directories(${JSON_INCLUDE_DIR})
endif()

# BlitzCC (The Transpiler)
add_executable(blitzcc src/compiler/blitzcc.cpp)

# Output name for compatibility
set_target_properties(blitzcc PROPERTIES OUTPUT_NAME "blitzcc")

# Define build systems
if(WIN32)
    set_target_properties(blitzcc PROPERTIES SUFFIX ".exe")
endif()

# Deploy to bin/ directory
set_target_properties(blitzcc PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

# SDL3 (SD3) configuration
set(SDL3_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/sd3" CACHE PATH "Path to SDL3")
if(EXISTS "${SDL3_DIR}")
    list(APPEND CMAKE_PREFIX_PATH "${SDL3_DIR}")
    find_package(SDL3 REQUIRED)
    target_link_libraries(blitzcc PRIVATE SDL3::SDL3)
    message(STATUS "SDL3 found at: ${SDL3_DIR}")
else()
    message(WARNING "SDL3 not found at ${SDL3_DIR}. Run build_windows.bat or build_linux.sh first.")
endif()

# Future: Runtime library modules
# add_library(bb_runtime STATIC src/libs/bb_core.cpp src/libs/bb_graphics.cpp ...)
