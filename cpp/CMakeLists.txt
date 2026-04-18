cmake_minimum_required(VERSION 3.14)
project(kit-vcs VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/W4)
endif()

find_package(OpenSSL REQUIRED)

include(FetchContent)
FetchContent_Declare(cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG v3.1.1)
FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip)
FetchContent_MakeAvailable(cxxopts googletest)

set(CORE_SOURCES
    src/utils/logger.cpp
    src/utils/hash.cpp
    src/utils/fs_utils.cpp
    src/utils/diff.cpp
    src/core/objects/blob.cpp
    src/core/objects/tree.cpp
    src/core/objects/commit.cpp
    src/core/index.cpp
    src/core/refs.cpp
    src/core/repository.cpp
    src/cli/commands/cmd_init.cpp
    src/cli/commands/cmd_add.cpp
    src/cli/commands/cmd_commit.cpp
    src/cli/commands/cmd_status.cpp
    src/cli/commands/cmd_log.cpp
    src/cli/commands/cmd_branch.cpp
    src/cli/commands/cmd_checkout.cpp
    src/cli/commands/cmd_diff.cpp
    src/cli/commands/cmd_merge.cpp
    src/cli/commands/cmd_reset.cpp
    src/cli/commands/cmd_stash.cpp
    src/cli/cli.cpp
)

add_library(kit_core STATIC ${CORE_SOURCES})
target_include_directories(kit_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(kit_core PUBLIC OpenSSL::Crypto cxxopts::cxxopts)

add_executable(kit-vcs src/main.cpp)
target_link_libraries(kit-vcs PRIVATE kit_core)

enable_testing()

file(GLOB UNIT_SOURCES "tests/unit/*.cpp")
if(UNIT_SOURCES)
    add_executable(test_unit ${UNIT_SOURCES})
    target_link_libraries(test_unit PRIVATE kit_core gtest gtest_main)
    target_include_directories(test_unit PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME unit COMMAND test_unit)
endif()

file(GLOB INTEGRATION_SOURCES "tests/integration/*.cpp")
if(INTEGRATION_SOURCES)
    add_executable(test_integration ${INTEGRATION_SOURCES})
    target_link_libraries(test_integration PRIVATE kit_core gtest gtest_main)
    target_include_directories(test_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(test_integration PRIVATE
        KIT_BINARY="$<TARGET_FILE:kit-vcs>")
    add_test(NAME integration COMMAND test_integration)
endif()

message(STATUS "Project: ${PROJECT_NAME} v${PROJECT_VERSION}")
