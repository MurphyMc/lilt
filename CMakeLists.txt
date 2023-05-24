cmake_minimum_required(VERSION 2.9)

# Here's a cute snippet from Marcus D. Hanwell at
# https://blog.kitware.com/cmake-and-the-default-build-type/
# It sets the default build type and also makes build type a
# selectable option in cmake GUIs.
# Set a default build type if none was specified
set(default_build_type "Release")
#if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
#  set(default_build_type "Debug")
#endif()
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(LILT_FONT_TINY "false" CACHE BOOL "Use tiny 4x8 font")
if (LILT_FONT_TINY)
  add_definitions(-DLILT_FONT_TINY)
endif ()

set(LILT_TITLE_SET "true" CACHE BOOL "Allow xterm window title setting")
if (LILT_TITLE_SET)
  add_definitions(-DLILT_TITLE_SET)
endif ()

set(TMT_MODE_MSGS "true" CACHE BOOL "libtmt has TMT_MSG_(UN)SETMODE")
if (TMT_MODE_MSGS)
  add_definitions(-DTMT_MODE_MSGS)
endif ()

if (DEFINED MAC_SDK_ROOT)
  set(CMAKE_C_FLAGS "--sysroot ${MAC_SDK_ROOT} ${CMAKE_C_FLAGS}")
endif ()

project (lilt)
add_executable(lilt
  lilt.c
  libtmt/tmt.c
)

find_package(SDL REQUIRED)
include_directories(${SDL_INCLUDE_DIR})

target_link_libraries(lilt ${SDL_LIBRARY})
