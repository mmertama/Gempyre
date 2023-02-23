#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#include <gtest/gtest.h>
#include "gempyre.h"
#include <memory>
#include <string>
#include <functional>

namespace GempyreTest {
    void killHeadless();
    class TestUi : public testing::Test {
public:
    void test(const std::function<void () >& f);
    static void SetUpTestSuite();
    static void TearDownTestSuite();
private:
    void SetUp() override;
    void TearDown() override;
public:
    static inline std::unique_ptr<Gempyre::Ui> m_ui;
    std::string m_current_test;
    };
}

#endif //__TEST_UTILS_H__
