cmake_minimum_required(VERSION 3.12)
project(traffic_sign_detection)

add_executable(traffic_sign_detection
    main.cpp
    # add other .cpp files here when needed.
)

target_compile_features(traffic_sign_detection PRIVATE cxx_std_20)
set_target_properties(traffic_sign_detection PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

target_link_libraries(traffic_sign_detection PRIVATE liboccar)
