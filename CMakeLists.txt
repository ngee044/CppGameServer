cmake_minimum_required(VERSION 3.18)

set(PROJECT_NAME CppGameServer)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set the output directory to the build directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(APPLE AND NOT CMAKE_CROSSCOMPILING)
    list(APPEND CMAKE_PREFIX_PATH /usr/local /opt/homebrew)
endif()

project(${PROJECT_NAME} VERSION 1.0.0.0)

add_subdirectory(.CppToolkit)

add_subdirectory(DummyClient)
add_subdirectory(DummyClientManager)
add_subdirectory(InfraService)
add_subdirectory(MainService)


