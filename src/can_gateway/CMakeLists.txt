cmake_minimum_required(VERSION 3.12)
project(can_gateway)

add_executable(can_gateway
    main.cpp
    ocCanGateway.cpp
)

target_compile_features(can_gateway PRIVATE cxx_std_20)
set_target_properties(can_gateway PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

target_link_libraries(can_gateway PRIVATE liboccar)
