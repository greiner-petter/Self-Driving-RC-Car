cmake_minimum_required(VERSION 3.12)
project(video_input)

find_package(OpenCV REQUIRED)

add_executable(video_input
    main.cpp
)

target_compile_features(video_input PRIVATE cxx_std_20)
set_target_properties(video_input PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

target_link_libraries(video_input PRIVATE liboccar ${OpenCV_LIBS})
