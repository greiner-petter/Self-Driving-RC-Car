cmake_minimum_required(VERSION 3.12)
project(video_recorder)

find_package(OpenCV REQUIRED)

add_executable(video_recorder
    main.cpp
)

target_compile_features(video_recorder PRIVATE cxx_std_20)
set_target_properties(video_recorder PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD 20
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

target_link_libraries(video_recorder PRIVATE liboccar ${OpenCV_LIBS})
