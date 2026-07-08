cmake_minimum_required(VERSION 3.16)
project(NavisRoutingEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_executable(navis
    src/main.cpp
    src/Parser.cpp
    src/Graph.cpp
)

target_include_directories(navis PRIVATE ${CMAKE_SOURCE_DIR}/include)


if (MSVC)
    target_compile_options(navis PRIVATE /W4)
else()
    target_compile_options(navis PRIVATE -Wall -Wextra -O2)
endif()

# Google Test Integration
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(navis_test
  tests/test_graph.cpp
  src/Graph.cpp
  src/Parser.cpp
)
target_include_directories(navis_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(navis_test gtest_main)

include(GoogleTest)
gtest_discover_tests(navis_test)


add_custom_command(TARGET navis POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:navis>/data
)
