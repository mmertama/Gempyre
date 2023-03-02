#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#include <gtest/gtest.h>
#include "gempyre.h"
#include <memory>
#include <string>
#include <functional>
#include <chrono>
#include <string_view>

using namespace std::chrono_literals;

namespace GempyreTest {
    void killHeadless();
    class TestUi : public testing::Test {
    public:
        static void SetUpTestSuite();
        static void TearDownTestSuite();
    public:
        void test(const std::function<void () >& f);
        void post_test(const std::function<void () >& f);
        void test_wait(std::chrono::milliseconds wait = 99h); // most compilers cannot wait std::hours/milli/seconds::max() due overflow!
        void timeout(std::chrono::milliseconds wait);
        void test_exit();
        Gempyre::Ui& ui();
        std::string_view current_test() const;
        TestUi();
        ~TestUi();
    private:
        void SetUp() override;
        void TearDown() override;
        void run();
        void exit();
        void finish();
    private:
        static inline std::unique_ptr<Gempyre::Ui> m_ui;
        std::string m_current_test;
    private:
        std::function<void()> m_postFunc = nullptr;
        std::function<void()> m_testFunc = nullptr;
        unsigned m_state;
    };
}

#endif //__TEST_UTILS_H__
