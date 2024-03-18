cmake_minimum_required(VERSION 3.12)
project(video_viewer)

add_executable(video_viewer
    main.cpp
)

target_compile_features(video_viewer PRIVATE cxx_std_20)
set_target_properties(video_viewer PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD 20
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

target_link_libraries(video_viewer PRIVATE liboccar)
