cmake_minimum_required(VERSION 3.12)

project(Ostfalia-Cup)

# Check compiler version
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10)
    message(FATAL_ERROR "You are using an unsupported version of G++. Must be 10 or newer. See docs for more help.")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10)
    message(FATAL_ERROR "You are using an unsupported version of Clang. Must be 10 or newer. See docs for more help.")
  endif()
endif()

# Compiler Options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wconversion")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")

# Linker options
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")

enable_testing()

# Include all processes that should be built
add_subdirectory(src/liboccar)
add_subdirectory(src/camera)
add_subdirectory(src/can_gateway)
add_subdirectory(src/command_line)

add_subdirectory(src/decider)

add_subdirectory(src/eth_gateway)
add_subdirectory(src/ipc_hub)
add_subdirectory(src/tachometer)
add_subdirectory(src/video_input)
add_subdirectory(src/video_recorder)
add_subdirectory(src/video_viewer)
add_subdirectory(src/virtual_car)
add_subdirectory(src/image_processing_bev)
add_subdirectory(src/lane_detection)
add_subdirectory(src/intersection_detection)
add_subdirectory(src/traffic_sign_detection)
