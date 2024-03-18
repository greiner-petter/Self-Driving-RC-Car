cmake_minimum_required(VERSION 3.12)
project(ipc_hub)

add_executable(ipc_hub
    main.cpp
    ipc_hub.cpp
)

target_compile_features(ipc_hub PRIVATE cxx_std_20)
set_target_properties(ipc_hub PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

target_link_libraries(ipc_hub PRIVATE liboccar)
