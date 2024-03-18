cmake_minimum_required(VERSION 3.12)
project(liboccar)

find_package(X11 REQUIRED)
find_package(OpenGL REQUIRED)

add_library(liboccar SHARED
    ../common/ocAlarm.cpp
    ../common/ocArgumentParser.cpp
    ../common/ocAssert.cpp
    ../common/ocBuffer.cpp
    ../common/ocBufferReader.cpp
    ../common/ocBufferWriter.cpp
    ../common/ocCar.cpp
    ../common/ocCarConfig.cpp
    ../common/ocCommon.cpp
    ../common/ocConfigFileReader.cpp
    ../common/ocFileWatcher.cpp
    ../common/ocGeometry.cpp
    ../common/ocImageOps.cpp
    ../common/ocIpcSocket.cpp
    ../common/ocLogger.cpp
    ../common/ocMember.cpp
    ../common/ocPollEngine.cpp
    ../common/ocProfiler.cpp
    ../common/ocQoiFormat.cpp
    ../common/ocTime.cpp
    ../common/ocTypes.cpp
    ../common/ocWindow.cpp
)

target_compile_features(liboccar PRIVATE cxx_std_20)
target_compile_options(liboccar PRIVATE -fno-semantic-interposition)
set_target_properties(liboccar PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../lib")

target_link_libraries(liboccar PRIVATE ${X11_LIBRARIES} OpenGL::OpenGL OpenGL::EGL OpenGL::GLU)

set(TEST_FILES
    ../common/tests/ocArgumentParser_test.cpp
    ../common/tests/ocArray_test.cpp
    ../common/tests/ocCommon_test.cpp
    ../common/tests/ocMat_test.cpp
    ../common/tests/ocPose_test.cpp
    ../common/tests/ocVec_test.cpp
)

foreach( test_file ${TEST_FILES} )

    get_filename_component(bin_name ${test_file} NAME_WE)
    add_executable(${bin_name} ${test_file})

    target_compile_features(${bin_name} PRIVATE cxx_std_20)
    set_target_properties(${bin_name} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin/tests")

    target_link_libraries(${bin_name} PRIVATE liboccar)
    add_test(${bin_name} ${CMAKE_BINARY_DIR}/../bin/tests/${bin_name})

endforeach()
