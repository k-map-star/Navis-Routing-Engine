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


add_custom_command(TARGET navis POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:navis>/data
)
