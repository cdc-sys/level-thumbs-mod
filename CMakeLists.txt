cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
	set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
	set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(LevelThumbnails VERSION 1.0.0)

file(GLOB_RECURSE SOURCES
    src/*.cpp
)
add_library(${PROJECT_NAME} SHARED
    ${SOURCES}
)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# add libwebp
CPMAddPackage(
  NAME libwebp
  GITHUB_REPOSITORY "prevter/libwebp"
  GIT_TAG "950daa7"
  OPTIONS
    "WEBP_BUILD_CWEBP OFF"
    "WEBP_BUILD_DWEBP OFF"
    "WEBP_BUILD_IMG2WEBP OFF"
    "WEBP_BUILD_WEBPINFO OFF"
    "WEBP_BUILD_WEBPMUX OFF"
    "WEBP_BUILD_GIF2WEBP OFF"
    "WEBP_BUILD_ANIM_UTILS OFF"
    "WEBP_BUILD_EXTRAS OFF"
)
target_compile_definitions(webp PRIVATE _Float16=float)
target_include_directories(${PROJECT_NAME} PRIVATE ${libwebp_SOURCE_DIR}/src)
target_link_libraries(${PROJECT_NAME} webp)

# add argon 
CPMAddPackage("gh:GlobedGD/argon@1.1.6")
target_link_libraries(${PROJECT_NAME} argon)

setup_geode_mod(${PROJECT_NAME} EXTERNALS geode.node-ids:1.7.1)
