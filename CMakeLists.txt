cmake_minimum_required(VERSION 3.11)

# ---------------------------------------------------------------------------------------
# Start Amot project
# ---------------------------------------------------------------------------------------

project(amot)

# ---------------------------------------------------------------------------------------
# Set default build to release
# ---------------------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif()

set(CMAKE_CXX_COMPILER "/usr/bin/g++-11")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-std=c++20 -fPIC -fno-strict-aliasing -Wall ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-g -O3 -DNDEBUG")

message("------------ Cxx flags -------------")
message("  CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}: ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}}")
message("------------------------------------")


include_directories("${PROJECT_SOURCE_DIR}")

# 编译的库在 Amot/lib 下生成
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
message("------------ LIBRARY_OUTPUT_PATH -------------")
message(${PROJECT_SOURCE_DIR}/lib)
message(${PROJECT_SOURCE_DIR}/bin)
message("----------------------------------------------")

if(NOT TARGET spdlog)
    find_package(spdlog REQUIRED)
endif()

include(CTest)
enable_testing()

if(BUILD_TESTING)
    add_subdirectory(test)
endif()


add_executable(amot_test test/amot_tests/test.cpp)
target_link_libraries(amot_test PRIVATE spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)