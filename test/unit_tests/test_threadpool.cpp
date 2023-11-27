#include <gtest/gtest.h>
#include <exception>

#include <chrono>
#include <functional>
#include <thread>
#include <vector>

#include "../unittest.h"
#include "amot/common/threadPool.h"

namespace amot {

class ThreadPoolTest : public FUTURE_TESTBASE {
public:
	std::shared_ptr<ThreadPool> _tp;

public:
	void caseSetUp() override {}
	void caseTearDown() override {}
};

TEST_F(ThreadPoolTest, testScheduleWithId) {
    _tp = std::make_shared<ThreadPool>(2);
    std::thread::id id1, id2, id3;
    std::atomic<bool> done1(false), done2(false), done3(false), done4(false);
    std::function<void()> f1 = [this, &done1, &id1]() {
        id1 = std::this_thread::get_id();
        ASSERT_EQ(_tp->getCurrentId(), 0);
        done1 = true;
    };
    std::function<void()> f2 = [this, &done2, &id2]() {
        id2 = std::this_thread::get_id();
        ASSERT_EQ(_tp->getCurrentId(), 0);
        done2 = true;
    };
    std::function<void()> f3 = [this, &done3, &id3]() {
        id3 = std::this_thread::get_id();
        ASSERT_EQ(_tp->getCurrentId(), 1);
        done3 = true;
    };
    std::function<void()> f4 = [&done4]() { done4 = true; };
    _tp->scheduleById(std::move(f1), 0);
    _tp->scheduleById(std::move(f2), 0);
    _tp->scheduleById(std::move(f3), 1);
    _tp->scheduleById(std::move(f4));

    while (!done1.load() || !done2.load() || !done3.load() || !done4.load())
        ;
    ASSERT_TRUE(id1 == id2) << id1 << " " << id2;
    ASSERT_TRUE(id1 != id3) << id1 << " " << id3;
    ASSERT_TRUE(_tp->getCurrentId() == -1);
}

}
