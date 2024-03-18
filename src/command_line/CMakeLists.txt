cmake_minimum_required(VERSION 3.12)
project(command_line)

add_executable(command_line
    main.cpp
)

target_compile_features(command_line PRIVATE cxx_std_20)
set_target_properties(command_line PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

target_link_libraries(command_line PRIVATE liboccar)
