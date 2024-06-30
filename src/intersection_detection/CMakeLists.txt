cmake_minimum_required(VERSION 3.12)
project(intersection_detection)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(intersection_detection
    main.cpp
    Histogram.cpp
    IntersectionClassification.cpp
    # add other .cpp files here when needed.
)

target_compile_features(intersection_detection PRIVATE cxx_std_20)
set_target_properties(intersection_detection PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

target_link_libraries(intersection_detection PRIVATE liboccar ${OpenCV_LIBS})
