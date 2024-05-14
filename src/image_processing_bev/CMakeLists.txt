cmake_minimum_required(VERSION 3.12)
project(image_processing_bev)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(image_processing_bev
    main.cpp
    # add other .cpp files here when needed.
)

target_compile_features(image_processing_bev PRIVATE cxx_std_20)
set_target_properties(image_processing_bev PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

target_link_libraries(image_processing_bev PRIVATE liboccar ${OpenCV_LIBS})
