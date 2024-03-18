cmake_minimum_required(VERSION 3.12)
project(camera)

find_library(UEYE_LIB ueye_api)
if(NOT UEYE_LIB)
  message("Camera driver not found. Camera process will not be built.")
else()
  add_executable(camera
    main.cpp
  )

  target_compile_features(camera PRIVATE cxx_std_20)
  set_target_properties(camera PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

  target_link_libraries(camera PRIVATE liboccar ${UEYE_LIB})
endif()
