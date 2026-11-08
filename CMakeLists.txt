cmake_minimum_required(VERSION 3.10)

project(MethodOtrazheniy LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/O2)
else()
    add_compile_options(-O3)
    find_package(Threads REQUIRED)

endif()

add_executable(solution main.cpp)

if(NOT MSVC)
    target_link_libraries(solution PRIVATE Threads::Threads)
endif()