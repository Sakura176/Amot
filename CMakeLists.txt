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

set(CMAKE_CXX_COMPILER "/usr/bin/g++")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "-std=c++23 -fPIC -fno-strict-aliasing -Wall ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-g -O3 -DNDEBUG")

message("------------ Cxx flags -------------")
message("  CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}: ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}}")
message("------------------------------------")

set(BUILD_TESTING true)
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
    message("COMPILE DEFINITIONS: SPDLOG_ACTIVE_LEVEL")
endif()


include_directories("${PROJECT_SOURCE_DIR}")

# 编译的库在 Amot/lib 下生成
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE})
message("------------ LIBRARY_OUTPUT_PATH -------------")
message(${PROJECT_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/lib)
message(${PROJECT_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/bin)
message("----------------------------------------------")

if(NOT TARGET spdlog)
    find_package(spdlog REQUIRED)
endif()

enable_testing()
if(BUILD_TESTING)
    add_subdirectory(test)
endif()

add_executable(step_test test/amot_tests/test_step8.cpp)
target_link_libraries(step_test PRIVATE spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)


# add_executable(test_step12 test/amot_tests/test_step12.cpp)
# target_link_libraries(test_step12 PRIVATE spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)
