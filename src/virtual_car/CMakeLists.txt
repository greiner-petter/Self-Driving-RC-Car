cmake_minimum_required(VERSION 3.12)
project(virtual_car)

find_library(OCL_LIB OpenCL)
if(NOT OCL_LIB)
  message("OpenCL not installed. Simulation process will not be built.")
else()
  add_executable(virtual_car
      main.cpp
      ocSimulationWorld.cpp
      ocSimCar.cpp
      ocOdeSolver.cpp
      ocTrackStore.cpp
  )

  target_compile_features(virtual_car PRIVATE cxx_std_20)
  set_target_properties(virtual_car PROPERTIES
      CXX_EXTENSIONS OFF
      CXX_STANDARD 20
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

  target_link_libraries(virtual_car PRIVATE liboccar ${OCL_LIB})
endif()
