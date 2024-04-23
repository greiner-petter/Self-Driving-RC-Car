cmake_minimum_required(VERSION 3.12)
project(decider)

add_executable(decider
    main.cpp
    Statemachine.cpp
    States/Crossing_3_Way_Left.cpp
    States/Crossing_3_Way_Right.cpp
    States/Crossing_3_Way_T.cpp
    States/Crossing_4_Way.cpp
    States/Is_At_Crossing.cpp
    States/Normal_Drive.cpp
    States/Approaching_Crossing.cpp
    # add other .cpp files here when needed.
)

target_compile_features(decider PRIVATE cxx_std_20)
set_target_properties(decider PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

target_link_libraries(decider PRIVATE liboccar)