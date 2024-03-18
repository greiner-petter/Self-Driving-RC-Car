cmake_minimum_required(VERSION 3.12)
project(tachometer)

find_package(OpenCV REQUIRED)

add_executable(tachometer
    main.cpp
)

target_compile_features(tachometer PRIVATE cxx_std_20)
set_target_properties(tachometer PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

target_link_libraries(tachometer PRIVATE liboccar ${OpenCV_LIBS})
