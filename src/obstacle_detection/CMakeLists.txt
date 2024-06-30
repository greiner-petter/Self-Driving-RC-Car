cmake_minimum_required(VERSION 3.12)
project(obstacle_detection)

find_package(OpenCV REQUIRED)
file(GLOB AllSources *.cpp)

add_executable(obstacle_detection ${AllSources})

target_compile_features(obstacle_detection PRIVATE cxx_std_20)
set_target_properties(obstacle_detection PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

target_link_libraries(obstacle_detection PRIVATE liboccar ${OpenCV_LIBS})
target_include_directories(obstacle_detection PRIVATE ${OpenCV_INCLUDE_DIRS})

include(CMakePrintHelpers)
cmake_print_variables(OpenCV_INCLUDE_DIRS)
