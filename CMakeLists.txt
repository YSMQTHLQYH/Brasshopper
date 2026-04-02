cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_STANDARD 11)

set(PRODUCTION_BUILD OFF CACHE BOOL "Set to ON for production builds, which will disable debug symbols and enable optimizations." FORCE)

if(PRODUCTION_BUILD)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else()
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
endif()

project("core")



# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# This assumes the SDL source is available in vendored/SDL
add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)

# Gather all source files in the src/core directory and its subdirectories.
file(GLOB_RECURSE CORE_SOURCE_FILES CONFIGURE_DEPENDS "src/core/*.c")

add_executable("${CMAKE_PROJECT_NAME}" ${CORE_SOURCE_FILES})

if(PRODUCTION_BUILD)
    # In production builds, we want to use a relative path for resources, since the executable will be run from the build directory.
    target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC RESOURCES_PATH="./resources/")
    target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC PRODUCTION_BUILD=1)
else()
    # In development builds, we want to use an absolute path for resources, since the executable will be run from the project root directory.
    target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/resources/")
    target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC PRODUCTION_BUILD=0)
endif()

# target_include_directories("${CMAKE_PROJECT_NAME}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")

# Link to the actual SDL3 library.
target_link_libraries("${CMAKE_PROJECT_NAME}" PRIVATE SDL3::SDL3)