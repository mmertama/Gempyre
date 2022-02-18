#include <gempyre.h>
#include <gempyre_utils.h>
#include <iostream>
#include <vector>
#include <tuple>
#include <functional>
#include <set>
#ifdef HAS_FS
#include <filesystem>
#endif
#include "apitests_resource.h"
#include <cassert>

#include <gtest/gtest.h>

#define TEST_FAIL assert(false)

TEST(TestMockUi, openPage_with_page_browser) {
    constexpr auto htmlPage = TEST_HTML;
#ifdef HAS_FS
    ASSERT_TRUE(std::filesystem::exists(htmlPage));
#else
    ASSERT_TRUE(GempyreUtils::fileExists(htmlPage));
#endif

    Gempyre::Ui::Filemap map;
    const auto url = Gempyre::Ui::addFile(map, htmlPage);

    gempyre_utils_assert_x(url, std::string("Cannot load ") + htmlPage);

    Gempyre::Ui ui(map, *url, TEST_BINARY, "");
    bool ok = false;
    ui.onOpen([&ui, &ok]() {
        ok = true;
        ui.exit();
    });
    ui.onError([](const auto& element, const auto& info) {
        std::cerr << element << " err:" << info; TEST_FAIL;
    });
    const auto raii_ex = GempyreUtils::waitExpire(10s, []() {TEST_FAIL;});
    ui.run();
    ASSERT_TRUE(ok);
}


TEST(TestMockUi, openPage_with_page) {
    constexpr auto htmlPage = TEST_HTML;
#ifdef HAS_FS
    ASSERT_TRUE(std::filesystem::exists(htmlPage));
#else
    ASSERT_TRUE(GempyreUtils::fileExists(htmlPage));
#endif

    Gempyre::Ui::Filemap map;
    const auto url = Gempyre::Ui::addFile(map, htmlPage);

    gempyre_utils_assert_x(url, std::string("Cannot load ") + htmlPage);
    Gempyre::Ui ui(map, *url, TEST_BINARY, "");
    bool ok = false;
    ui.onOpen([&ui, &ok]() {
        ok = true;
        ui.exit();
    });
    ui.onError([](const auto& element, const auto& info){std::cerr << element << " err:" << info; TEST_FAIL;});
    const auto raii_ex = GempyreUtils::waitExpire(10s, []() {ASSERT_TRUE(false);});
    ui.run();
    ASSERT_TRUE(ok);
}


TEST(TestMockUi, openPage_with_browser) {
    Gempyre::Ui ui({{"/foobar.html", Apitestshtml}}, "foobar.html", TEST_BINARY, "");
    bool ok = false;
    ui.onOpen([&ui, &ok]() {
        ok = true;
        ui.exit();
    });
    ui.onError([](const auto& element, const auto& info){std::cerr << element << " err:" << info; TEST_FAIL;});
    const auto raii_ex = GempyreUtils::waitExpire(10s, []() {ASSERT_TRUE(false);});
    ui.run();
    ASSERT_TRUE(ok);

}

#define TEST_UI Gempyre::Ui ui(Apitests_resourceh, "apitests.html", TEST_BINARY, "")

TEST(TestMockUi, openPage_defaults) {
    TEST_UI;
    bool ok = false;
    ui.onOpen([&ui, &ok]() {
        ASSERT_FALSE(ok);
        ok = true;
        ui.exit();
    });
    ui.onError([](const auto& element, const auto& info){std::cerr << element << " err:" << info; TEST_FAIL;});
    const auto raii_ex = GempyreUtils::waitExpire(10s, []() {TEST_FAIL;});
    ui.run();
    ASSERT_TRUE(ok);
}

TEST(TestMockUi, onExit) {
    TEST_UI;
    bool ok = false;
    ui.onOpen([&ui, &ok]() {
        ASSERT_FALSE(ok);
        ui.exit();
    });
    ui.onExit([&ok]() {
        ASSERT_FALSE(ok);
        ok = true;
    });
    const auto raii_ex = GempyreUtils::waitExpire(10s, []() {TEST_FAIL;});
    ui.run();
    ASSERT_TRUE(ok);
}

TEST(TestMockUi, close) {
    TEST_UI;
    bool ok = false;
    ui.onOpen([&ui]() {
        ui.close();
    });
    ui.onExit([&ok]() {
        ASSERT_FALSE(ok);
        ok = true;
    });
    const auto raii_ex = GempyreUtils::waitExpire(10s, []() {TEST_FAIL;});
    ui.run();
    ASSERT_TRUE(ok);
}


TEST(TestMockUi, setLogging) {
    TEST_UI;
    ui.onOpen([&ui]() {
        ui.setLogging(true);
        ui.setLogging(false);
        ui.exit();
    });
    const auto raii_ex = GempyreUtils::waitExpire(10s, []() {TEST_FAIL;});
    ui.run();
}

TEST(TestMockUi, debug) {
    TEST_UI;
    bool ok = false;
    ui.onOpen([&ui, &ok]() {
        ui.debug("Test - Debug");
        ok = true;
        ui.exit();
    });
    const auto raii_ex = GempyreUtils::waitExpire(10s, []() {TEST_FAIL;});
    ui.run();
    ASSERT_TRUE(ok);
}


TEST(TestMockUi, alert) {
    TEST_UI;
    bool ok = false;
    ui.onOpen([&ui, &ok]() {
        ui.alert("Test - Alert");
        ok = true;
        ui.exit();
    });
    const auto raii_ex = GempyreUtils::waitExpire(10s, []() {TEST_FAIL;});
    ui.run();
    ASSERT_TRUE(ok);
}

TEST(TestMockUi, open) {
    TEST_UI;
    bool ok = false;
    ui.onOpen([&ui, &ok]() {
        ui.open("http://www.google.com");
        ok = true;
        ui.exit();
    });
    const auto raii_ex = GempyreUtils::waitExpire(10s, []() {TEST_FAIL;});
    ui.run();
    ASSERT_TRUE(ok);
}

TEST(TestMockUi, startTimer) {
    TEST_UI;
    bool ok = false;
    ui.after(1000ms, [&ui, &ok](Gempyre::Ui::TimerId id)  {
       (void)id;
       ui.exit();
       ok = true;
    });
    ui.run();
    ASSERT_TRUE(ok);
}

TEST(TestMockUi, startTimerNoId) {
    TEST_UI;
    bool ok = false;
    ui.after(1000ms, [&ui, &ok]()  {
       ui.exit();
       ok = true;
    });
    ui.run();
    ASSERT_TRUE(ok);
}

TEST(TestMockUi, stopTimer) {
    TEST_UI;
    bool ok = true;
    auto id = ui.after(1000ms, [&ok, &ui]()  {
       ok = false;
       ui.exit();
    });
    ui.after(3000ms, [&ui]() {
          ui.exit();
       });
    ui.cancelTimer(id);
    ui.run();
    EXPECT_TRUE(ok);
}


TEST(TestMockUi, startManyTimers) {
    TEST_UI;
    std::string test = "";
    ui.onOpen([&test](){
        test += 'm';
    });
    ui.after(0ms, [&test](Gempyre::Ui::TimerId id)  {
       (void)id;
       test += 'o';
    });
    ui.after(1ms, [&test](Gempyre::Ui::TimerId id)  {
       (void)id;
       test += 'n';
    });
    ui.after(100ms, [&test](Gempyre::Ui::TimerId id)  {
       (void)id;
       test += 's';
    });
    ui.after(1000ms, [&test](Gempyre::Ui::TimerId id)  {
       (void)id;
       test += 't';
    });
    ui.after(1001ms, [&test](Gempyre::Ui::TimerId id)  {
       (void)id;
       test += 'e';
    });
    ui.after(10002ms, [&test, &ui](Gempyre::Ui::TimerId id)  {
       (void)id;
       test += 'r';
       ui.exit();
    });
    ui.run();
    EXPECT_EQ("monster", test);
}

TEST(TestMockUi, timing) {
    TEST_UI;
    const auto start = std::chrono::system_clock::now();
    ui.after(1000ms, [&start]()  {
        const auto end = std::chrono::system_clock::now();
        const auto diff = end - start;
        EXPECT_TRUE(diff >= 1000ms);
    });
    ui.after(2000ms, [&start]()  {
        const auto end = std::chrono::system_clock::now();
        const auto diff = end - start;
        EXPECT_TRUE(diff >= 2000ms);
    });
    ui.after(4000ms, [&start, &ui]()  {
        const auto end = std::chrono::system_clock::now();
        const auto diff = end - start;
        EXPECT_TRUE(diff >= 4000ms);
        ui.exit();
    });
   ui.run();
}

TEST(TestMockUi, timerStartStop) {
    TEST_UI;
    int count = 0;
    ui.after(0s, [&](){
        ui.exit();
        ++count;
    });
    ui.run();
    std::this_thread::sleep_for(1s);
    ui.after(0s, [&](){
        ui.exit();
        ++count;
    });
    ui.run();
    EXPECT_EQ(count, 2);
}

TEST(TestMockUi, ping) {
    TEST_UI;
    bool ok = false;
    ui.after(1s, [&ok, &ui](){
        const auto ping = ui.ping();
        ok = ping.has_value() &&
        ping->first.count() >= 0 &&
        ping->second.count() >= 0 &&
        ping->first.count() < 30000 &&
        ping->second.count() < 30000;
        if(ping) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Ping:", ping->first.count(), ping->second.count());
            if(!ok)
                GempyreUtils::log(GempyreUtils::LogLevel::Error, "Ping too slow:", ping->first.count(), ping->second.count());
        }
        else
             GempyreUtils::log(GempyreUtils::LogLevel::Error, "Ping: N/A");
        ui.exit();
    });
    ui.run();
    EXPECT_TRUE(ok);
}

TEST(TestMockUi, resource) {
    TEST_UI;
    const auto r = ui.resource("/apitests.html");
    ASSERT_TRUE(r);
    const std::string html = GempyreUtils::join(*r);
    const auto p1 = html.find("html");
    const auto p2 = html.find("html");
    ASSERT_EQ(p1, p2);
}

TEST(TestMockUi, addFile) {
     TEST_UI;
    const std::string test = "The quick brown fox jumps over the lazy dog";
    const auto tempFile = GempyreUtils::writeToTemp(test);
    const auto ok = ui.addFile("test_data", tempFile);
    ASSERT_TRUE(ok);
    GempyreUtils::removeFile(tempFile);
    const auto r = ui.resource("test_data");
    const std::string text = GempyreUtils::join(*r);
    const auto p1 = text.find("quick");
    EXPECT_NE(p1, std::string::npos);
    EXPECT_EQ(text.length(), test.length());
}


TEST(TestMockUi, idTest) {
    TEST_UI;
    Gempyre::Element foo(ui, "test-1");
    ASSERT_EQ(foo.id(), "test-1");
}

TEST(TestMockUi, root) {
     TEST_UI;
    EXPECT_EQ(ui.root().id(), ""); //root has no id
}
