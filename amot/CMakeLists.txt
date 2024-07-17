
if(NOT TARGET spdlog)
    find_package(spdlog REQUIRED)
endif()

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
endif()


set(LIB_SRC
    amot/common/epoller.cpp
    )

add_library(amot SHARED ${LIB_SRC})

set(LIB_LIB
    amot)

target_link_libraries(amot PRIVATE spdlog::spdlog GTest::gtest_main GTest::gtest $<$<BOOL:${MINGW}>:ws2_32>)
