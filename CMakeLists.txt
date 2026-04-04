cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_STANDARD 11)

set(PRODUCTION_BUILD OFF CACHE BOOL "Set to ON for production builds, which will disable debug symbols and enable optimizations." FORCE)

if(PRODUCTION_BUILD)
    # In production builds, we want to use a relative path for resources, since the executable will be run from the build directory.
    add_compile_definitions(RESOURCES_PATH="./resources/")
    add_compile_definitions(PRODUCTION_BUILD=1)
    # While we are at it enable link time optimizations
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else()
    # In development builds, we want to use an absolute path for resources, since the executable will be run from the project root directory.
    add_compile_definitions(RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/resources/")
    add_compile_definitions(PRODUCTION_BUILD=0)
    # Don't bother with optimizations at link time
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
endif()


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG)
endif()
if(MSVC)
#MSVC is very stoopid and gives compiler warnings for null pointer on ASSERT(ptr != NULL)
#It DOESN'T give the warning without the assert btw, it straight up encourages not using asserts??
    add_compile_options(/wd6011)
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



# target_include_directories("${CMAKE_PROJECT_NAME}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")

# Link to the actual SDL3 library.
target_link_libraries("${CMAKE_PROJECT_NAME}" PRIVATE SDL3::SDL3)