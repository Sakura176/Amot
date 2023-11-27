#pragma once

#include <string>
#include <typeinfo>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class FUTURE_TESTBASE : public testing::Test {
public:
    virtual void caseSetUp() = 0;
    virtual void caseTearDown() = 0;

    void SetUp() override { caseSetUp(); }

    void TearDown() override { caseTearDown(); }
};

