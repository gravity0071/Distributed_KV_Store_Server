cmake_minimum_required(VERSION 3.10)

project(Distributed_KV_Store_Master VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Gather source files
file(GLOB UTIL_SOURCES "util/*.cpp")
file(GLOB TEST_SOURCES "tests/*.cpp")
file(GLOB CLIENT_SOURCES "handleClient/*.cpp")
file(GLOB HEART_SOURCES "handleHeartbeat/*.cpp")
file(GLOB COMMAND_SOURCES "handleLocalCommand/*.cpp")

# Fetch and configure nlohmann_json
include(FetchContent)
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.10.5
)
FetchContent_MakeAvailable(nlohmann_json)

# Fetch and configure GoogleTest
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

enable_testing()

# Create shared sources library
add_library(SharedSources OBJECT
        ${UTIL_SOURCES}
        ${CLIENT_SOURCES}
        ${HEART_SOURCES}
        ${COMMAND_SOURCES}
)

target_include_directories(SharedSources PRIVATE
        ${CMAKE_SOURCE_DIR}/util
        ${CMAKE_SOURCE_DIR}/handleClient
        ${CMAKE_SOURCE_DIR}/handleHeartbeat
        ${CMAKE_SOURCE_DIR}/handleLocalCommand
)

target_link_libraries(SharedSources PRIVATE nlohmann_json::nlohmann_json OpenSSL::SSL OpenSSL::Crypto)

# Main executable
add_executable(MyExecutable main.cpp $<TARGET_OBJECTS:SharedSources>)
target_link_libraries(MyExecutable PRIVATE nlohmann_json::nlohmann_json OpenSSL::SSL OpenSSL::Crypto)

# Add test executables for each test source
foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    set(TEST_TARGET "Test_${TEST_NAME}")

    add_executable(${TEST_TARGET} ${TEST_SOURCE} $<TARGET_OBJECTS:SharedSources>
            tests/testSendingReceivingDataBetweenTwoServers.cpp)
    target_link_libraries(${TEST_TARGET} PRIVATE gtest_main gmock_main OpenSSL::SSL OpenSSL::Crypto nlohmann_json::nlohmann_json)

    add_test(NAME ${TEST_TARGET} COMMAND ${TEST_TARGET})
endforeach()