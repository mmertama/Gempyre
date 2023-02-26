#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#include <gtest/gtest.h>
#include "gempyre.h"
#include <memory>
#include <string>
#include <functional>
#include <chrono>

using namespace std::chrono_literals;

namespace GempyreTest {
    void killHeadless();
    class TestUi : public testing::Test {
public:
    void test(const std::function<void () >& f);
    void post_test(const std::function<void () >& f);
    void test_wait(std::chrono::milliseconds wait = std::chrono::milliseconds::max());
    void timeout(std::chrono::milliseconds wait);
private:
    void SetUp() override;
    void TearDown() override;
public:
    std::unique_ptr<Gempyre::Ui> m_ui;
    std::string m_current_test;
private:
    std::function<void()> m_postFunc = nullptr;
    std::function<void()> m_testFunc = nullptr;
    unsigned m_state;
    };
}

#endif //__TEST_UTILS_H__
