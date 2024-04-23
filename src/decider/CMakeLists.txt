cmake_minimum_required(VERSION 3.12)
project(decider)

add_executable(decider
    main.cpp
    States/Statemachine.cpp
    States/Crossing_3_Way_Left.cpp
    # add other .cpp files here when needed.
)

target_compile_features(decider PRIVATE cxx_std_20)
set_target_properties(decider PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

target_link_libraries(decider PRIVATE liboccar)