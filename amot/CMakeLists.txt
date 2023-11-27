
if(NOT TARGET spdlog)
    find_package(spdlog REQUIRED)
endif()

set(LIB_SRC
    amot/common/epoller.cpp
    )

add_library(amot SHARED ${LIB_SRC})

set(LIB_LIB
    amot)

target_link_libraries(amot PRIVATE spdlog::spdlog GTest::gtest_main GTest::gtest $<$<BOOL:${MINGW}>:ws2_32>)
