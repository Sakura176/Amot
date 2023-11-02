cmake_minimum_required(VERSION 3.11)

# ---------------------------------------------------------------------------------------
# Start Amot project
# ---------------------------------------------------------------------------------------

# project(Amot VERSION ${SPDLOG_VERSION} LANGUAGES CXX)
project(amot)
# message(STATUS "Build spdlog: ${SPDLOG_VERSION}")

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
    # Stand-alone build
    find_package(spdlog REQUIRED)
endif()

add_executable(test_log test/test_log/test.cpp)
target_link_libraries(test_log PRIVATE spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)

add_executable(test_task test/test_common/test_task.cpp)
target_link_libraries(test_task PRIVATE spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)