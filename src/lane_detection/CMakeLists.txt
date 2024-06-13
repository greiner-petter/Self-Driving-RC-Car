cmake_minimum_required(VERSION 3.12)
project(lane_detection)

find_package(OpenCV REQUIRED)
include_directories(
  ${OpenCV_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../tensorflow/
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../flatbuffers/include/
)

ADD_LIBRARY(tensorflowlite SHARED IMPORTED)
set_property(TARGET tensorflowlite PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../../../tflite/tflite-dist/libs/linux_x64/libtensorflowlite.so)

add_executable(lane_detection
    main.cpp
)

target_compile_features(lane_detection PRIVATE cxx_std_20)
set_target_properties(lane_detection PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

target_link_libraries(lane_detection PRIVATE liboccar ${OpenCV_LIBS} tensorflowlite)