cmake_minimum_required(VERSION 3.12)
project(traffic_sign_detection)

find_package(OpenCV REQUIRED)
file(GLOB AllSources *.cpp)

add_executable(traffic_sign_detection ${AllSources})

target_compile_features(traffic_sign_detection PRIVATE cxx_std_20)
set_target_properties(traffic_sign_detection PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

target_link_libraries(traffic_sign_detection PRIVATE liboccar ${OpenCV_LIBS})
target_include_directories(traffic_sign_detection PRIVATE ${OpenCV_INCLUDE_DIRS})

include(CMakePrintHelpers)
cmake_print_variables(OpenCV_INCLUDE_DIRS)
