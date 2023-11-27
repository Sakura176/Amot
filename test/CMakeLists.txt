# 使用 GTest 框架
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/amot)

# 添加测试文件
add_executable(test_threadpool unit_tests/test_threadpool.cpp)

# 链接 GTest 库和你的源文件
target_link_libraries(test_threadpool PRIVATE GTest::GTest GTest::Main pthread)

# # 如果你的测试需要访问项目的源代码，可以添加以下行
# target_include_directories(test ${CMAKE_SOURCE_DIR}/test_common)

# 添加测试
# add_test(NAME test_threadpool COMMAND test_threadpool)
include(GoogleTest)
gtest_add_tests(TARGET test_threadpool)
