#include <gempyre.h>
#include <gempyre_utils.h>
#include <iostream>
#include <vector>
#include <tuple>
#include <functional>
#include <set>
#include <thread>
#ifdef HAS_FS
#include <filesystem>
#endif
#include "apitests_resource.h"
#include <cassert>
#include <iostream>

#include <gtest/gtest.h>

//#define TEST_FAIL assert(false)

#ifdef RASPBERRY_OS
constexpr auto WaitExpireTimeout = 30s;
#else
constexpr auto WaitExpireTimeout = 20s;
#endif

#define PRINT_D(_X) std::cout << "\033[0;35m" << "[          ] " << "\033[0;0m" << _X << std::endl;


//#define HANDLE_EXPIRE ui.exit()
#define HANDLE_EXPIRE  GempyreUtils::log(GempyreUtils::LogLevel::Error, "Annoying!"); ADD_FAILURE() << "Chrome not start! o:" << (ok ? "opn": "nopn")<< " ui:" << (ui.ui_available() ? "ok" : "nok") << " t:" << (ui.is_timer_on_hold() ? "hold" : "nold"); std::abort();

TEST(TestMockUi, openPage_with_page_browser) {
    constexpr auto htmlPage = TEST_HTML;
#ifdef HAS_FS
    ASSERT_TRUE(std::filesystem::exists(htmlPage));
#else
    ASSERT_TRUE(GempyreUtils::file_exists(htmlPage));
#endif

    Gempyre::Ui::FileMap map;
    const auto url = Gempyre::Ui::add_file(map, htmlPage);

    gempyre_utils_assert_x(url, std::string("Cannot load ") + htmlPage);

    Gempyre::Ui ui(map, *url, TEST_BINARY, "");
    bool ok = false;
    ui.on_open([&ui, &ok]() {
        PRINT_D("Test open 1");
        ok = true;
        ui.exit();
    });
    ui.on_error([](const auto& element, const auto& info) {
        FAIL() << element << " err:" << info;
    });
    const auto raii_ex = GempyreUtils::wait_expire(WaitExpireTimeout, [&]() {
        HANDLE_EXPIRE;
        });
    ui.run();
    ASSERT_TRUE(ok);
}


TEST(TestMockUi, openPage_with_page) {
    constexpr auto htmlPage = TEST_HTML;
#ifdef HAS_FS
    ASSERT_TRUE(std::filesystem::exists(htmlPage));
#else
    ASSERT_TRUE(GempyreUtils::file_exists(htmlPage));
#endif

    Gempyre::Ui::FileMap map;
    const auto url = Gempyre::Ui::add_file(map, htmlPage);

    gempyre_utils_assert_x(url, std::string("Cannot load ") + htmlPage);
    Gempyre::Ui ui(map, *url, TEST_BINARY, "");
    bool ok = false;
    ui.on_open([&ui, &ok]() {
        PRINT_D("Test open 2");
        ok = true;
        ui.exit();
    });
    ui.on_error([](const auto& element, const auto& info){
        FAIL() << element << " err:" << info;
        });
    const auto raii_ex = GempyreUtils::wait_expire(WaitExpireTimeout, [&]() {
        FAIL() << "timeout";
        HANDLE_EXPIRE;
    });
    ui.run();
    ASSERT_TRUE(ok);
}


TEST(TestMockUi, openPage_with_browser) {
    Gempyre::Ui ui({{"/foobar.html", Apitestshtml}}, "foobar.html", TEST_BINARY, "");
    bool ok = false;
    ui.on_open([&ui, &ok]() {
        PRINT_D("Test open 3");
        ok = true;
        ui.exit();
    });
    ui.on_error([](const auto& element, const auto& info){FAIL() << element << " err:" << info;});
    const auto raii_ex = GempyreUtils::wait_expire(WaitExpireTimeout, [&]() {
        FAIL() << "timeout!";
        HANDLE_EXPIRE;
        });
    ui.run();
    ASSERT_TRUE(ok);

}

#define TEST_UI Gempyre::Ui ui(Apitests_resourceh, "apitests.html", TEST_BINARY, "")

TEST(TestMockUi, openPage_defaults) {
    TEST_UI;
    bool ok = false;
    ui.on_open([&ui, &ok]() {
        PRINT_D("Test open 4");
        ASSERT_FALSE(ok);
        ok = true;
        ui.exit();
    });
    ui.on_error([](const auto& element, const auto& info){FAIL() << element << " err:" << info;});
    const auto raii_ex = GempyreUtils::wait_expire(WaitExpireTimeout, [&]() { 
        HANDLE_EXPIRE;
        });
    ui.run();
    ASSERT_TRUE(ok);
}

TEST(TestMockUi, onExit) {
    TEST_UI;
    bool ok = false;
    ui.on_open([&ui, &ok]() {
        ASSERT_FALSE(ok);
        ui.exit();
    });
    ui.on_exit([&ok]() {
         PRINT_D("Test open 5");
        ASSERT_FALSE(ok);
        ok = true;
    });
    const auto raii_ex = GempyreUtils::wait_expire(WaitExpireTimeout, [&]() { 
        HANDLE_EXPIRE;
        });
    ui.run();
    ASSERT_TRUE(ok);
}

/*
TEST(TestMockUi, close) {
    TEST_UI;
    bool ok = false;
    ui.on_open([&ui]() {
        ui.close();
    });
    ui.on_exit([&ok]() {
        ASSERT_FALSE(ok);
        ok = true;
    });
    const auto raii_ex = GempyreUtils::wait_expire(WaitExpireTimeout, []() { FAIL() << "Expired!";});
    ui.run();
    ASSERT_TRUE(ok);
}
*/

#ifdef HAS_MOCK // real browser would block without user action
TEST(TestMockUi, alert) {
    TEST_UI;
    bool ok = false;
    ui.on_open([&ui, &ok]() {
        PRINT_D("Test open 6");
        ui.alert("Test - Alert");
        ok = true;
        ui.exit();
    });
    const auto raii_ex = GempyreUtils::wait_expire(WaitExpireTimeout, [&]() {
        HANDLE_EXPIRE;
    });
    ui.run();
    ASSERT_TRUE(ok);
}
#endif

//#ifndef RASPBERRY_OS

TEST(TestMockUi, open) {
    TEST_UI;
    bool ok = false;
    ui.on_open([&ui, &ok]() {
        PRINT_D("Test open 7");
        ui.open("http://www.google.com");
        ok = true;
        ui.exit();
    });
    const auto raii_ex = GempyreUtils::wait_expire(WaitExpireTimeout, [&]() { 
        HANDLE_EXPIRE;
        });
    ui.run();
    ASSERT_TRUE(ok);
}

//#endif


/*
// why eval breaks TestUi tests?
// why eval breaks this this?
TEST(TestMockUi, eval) {
    TEST_UI;
    ui.eval("document.write('<h3 id=\\\"foo\\\">Bar</h3>')");
    ui.on_open([&ui]() {
        Gempyre::Element el(ui, "foo");
        const auto html = el.html();
        EXPECT_TRUE(html);
        ASSERT_TRUE(html.value() == "Bar");
        ui.exit();
    });
    ui.run();
}
*/


