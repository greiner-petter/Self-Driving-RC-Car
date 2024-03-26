cmake_minimum_required(VERSION 3.12)
project(simple_drive)

add_executable(simple_drive
    main.cpp
    # add other .cpp files here when needed.
)

target_compile_features(simple_drive PRIVATE cxx_std_20)
set_target_properties(simple_drive PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

target_link_libraries(simple_drive PRIVATE liboccar)
