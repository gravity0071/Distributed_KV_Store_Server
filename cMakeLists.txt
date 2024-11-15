cmake_minimum_required(VERSION 3.10)

project(Distributed_KV_Store_Master VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB UTIL_SOURCES "util/*.cpp")
file(GLOB TEST_SOURCES "tests/*.cpp")
file(GLOB CLIENT_SOURCES "handleClient/*.cpp")
file(GLOB HEART_SOURCES "handleHeartbeat/*.cpp")
file(GLOB COMMAND_SOURCES "handleLocalCommand/*.cpp")

add_executable(MyExecutable main.cpp
        ${UTIL_SOURCES}
        ${CLIENT_SOURCES}
        ${HEART_SOURCES}
        ${COMMAND_SOURCES})

include(FetchContent)
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.10.5
)
FetchContent_MakeAvailable(nlohmann_json)

target_link_libraries(MyExecutable PRIVATE nlohmann_json::nlohmann_json)

include_directories(
        ${CMAKE_SOURCE_DIR}/util
        ${CMAKE_SOURCE_DIR}/handleClient
        ${CMAKE_SOURCE_DIR}/handleHeartbeat
        ${CMAKE_SOURCE_DIR}/handleLocalCommand
)

# Add GoogleTest and GoogleMock
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Add test executable for ClientThread and other tests
add_executable(KVStoreMapTest
        ${TEST_SOURCES}
        ${UTIL_SOURCES}
        ${CLIENT_SOURCES}
        ${HEART_SOURCES}
        ${COMMAND_SOURCES}
        )

# Link GoogleTest and GoogleMock libraries
target_link_libraries(KVStoreMapTest gtest_main gmock_main nlohmann_json::nlohmann_json)

foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

    add_executable(${TEST_NAME} ${TEST_SOURCE} ${UTIL_SOURCES} ${CLIENT_SOURCES} ${HEART_SOURCES} ${COMMAND_SOURCES})

    target_link_libraries(${TEST_NAME} PRIVATE gtest_main gmock_main nlohmann_json::nlohmann_json)

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()