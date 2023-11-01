#include "gempyre_test.h"
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
#include <cassert>
#include <thread>

using namespace std::chrono_literals;
using namespace GempyreTest;


// That was FISHY: I had had to more this test on top here as it fails if that last (after eval or so)
// ... or was there something that eval has to be last?
// cannot remember, but anyway the tests are independent! (what is FISHY as a trout)
TEST_F(TestUi, parent) {
    test([this]() {
        EXPECT_FALSE(ui().root().parent());
        const Gempyre::Element t1 (ui(), "test-1");
        const auto parent = t1.parent();
        ASSERT_TRUE(parent.has_value());
        const auto root = ui().root();
        EXPECT_EQ(parent->id(), root.id());
        Gempyre::Element t2(ui(), "test-child-0");
        const auto parent2 = t2.parent();
        ASSERT_TRUE(parent2.has_value());
        EXPECT_EQ(parent2->id(), t1.id());
    });
}


TEST_F(TestUi, onReload) {
    /** window.location.reload is deprecated **/
    test([](){});
}

TEST_F(TestUi, addressOf) {
    const auto htmlPage = TEST_HTML;
#ifdef HAS_FS
    ASSERT_TRUE(std::filesystem::exists(htmlPage));
#else
    ASSERT_TRUE(GempyreUtils::file_exists(htmlPage));
#endif
    test([htmlPage, this]() {
        ASSERT_TRUE(ui().address_of(htmlPage).length() > 0); //TODO better test would be write this as html and open it
    });
}

TEST_F(TestUi, root) {;
    EXPECT_EQ(ui().root().id(), ""); //root has no id
}



TEST_F(TestUi, setLogging) {
    ui().set_logging(true);
    ui().set_logging(false);
}

TEST_F(TestUi, debug) {
    ui().debug("Test - Debug");
}


TEST_F(TestUi, byClass) {
    test([this]() {
        const auto classes = ui().by_class("test-class");
        ASSERT_TRUE(classes);
        ASSERT_EQ(classes->size(), 4); //4 test-classes
    });
}


TEST_F(TestUi, idTest) {
    Gempyre::Element foo(ui(), "test-1");
    ASSERT_EQ(foo.id(), "test-1");
}

TEST_F(TestUi, byName) {
    test([this]() {
        const auto names = ui().by_name("test-name");
        ASSERT_TRUE(names.has_value());
        ASSERT_EQ(names->size(), 5); //5 test-classes
    });
}

TEST_F(TestUi, devicePixelRatio) {
    test([this]() {
        const auto dr = ui().device_pixel_ratio();
        ASSERT_TRUE(dr);
        ASSERT_GT(*dr, 0);
    });
}

TEST_F(TestUi, ElementCreate) {
    Gempyre::Element parent(ui(), "test-1");
    Gempyre::Element(ui(), "boing_1", "div", parent);
    test([parent]() {
        const auto cop = parent.children();
        ASSERT_TRUE(cop.has_value());
        ASSERT_NE(std::find_if(cop->begin(), cop->end(), [](const auto el ){return el.id() == "boing_1";}), cop->end());
    });
}

TEST_F(TestUi, ElementCreateLater) {
    test([this]() {
        Gempyre::Element parent(ui(), "test-1");
        Gempyre::Element(ui(), "boing_2", "div", parent);
        const auto cop = parent.children();
        ASSERT_TRUE(cop.has_value());
        ASSERT_NE(std::find_if(cop->begin(), cop->end(), [](const auto el ){return el.id() == "boing_2";}), cop->end());
    });
}

TEST_F(TestUi, subscribe) {
    Gempyre::Element el(ui(), "test-1");
    const auto el_id = el.id();
    static bool is_open = false;
    ui().after(1s, [&]() {
        is_open = true;
    });
    el.subscribe("test_event", [el_id, this](const Gempyre::Event& eel) {
        ASSERT_EQ(eel.element.id(), el_id);
        ASSERT_TRUE(is_open);
        test_exit();
    });
    ui().after(2s, [&]()  {
          el.set_attribute("style", "color:green");
       });
    test_wait();
    ASSERT_TRUE(is_open);
}

TEST_F(TestUi, setHTML) {
    Gempyre::Element el(ui(), "test-1");
    el.set_html("Test-dyn");
    test([el]() {
        const auto html = el.html();
        ASSERT_TRUE(html.has_value());
        ASSERT_EQ(html.value(), "Test-dyn");
    });
}

TEST_F(TestUi, setAttribute) {
    Gempyre::Element el(ui(), "test-1");
    el.set_attribute("value", "Test-attr-dyn");
    test([el]() {
        const auto attrs = el.attributes();
        ASSERT_TRUE(attrs.has_value());
        ASSERT_NE(attrs->find("value"), attrs->end());
        ASSERT_EQ(attrs.value().find("value")->second, "Test-attr-dyn");
    });
}

TEST_F(TestUi, setBoolAttribute) {
    Gempyre::Element el(ui(), "test-1");
    el.set_attribute("value");
    test([el]() {
        const auto attrs = el.attributes();
        ASSERT_TRUE(attrs.has_value());
        ASSERT_NE(attrs->find("value"), attrs->end());
        ASSERT_EQ(attrs.value().find("value")->second, "true");
    });
}


TEST_F(TestUi, remove) {
    test([this]() {
        Gempyre::Element el(ui(), "test-2");
        const auto chlds = el.children();
        ASSERT_TRUE(chlds.has_value());
        ASSERT_GT(chlds->size(), 3);
        ASSERT_EQ((*chlds)[3].id(), "test-child-7");
        Gempyre::Element c(ui(),"test-child-7");
        c.remove();
        const auto cop = el.children();
        ASSERT_TRUE(cop.has_value());
        ASSERT_EQ(std::find_if(cop->begin(), cop->end(), [](const auto el ){return el.id() == "test-child-7";}), cop->end());
    });
}

TEST_F(TestUi, removeAttribute) {
    test([this]() {
        Gempyre::Element el(ui(), "hidden");
        auto attrs0 = el.attributes();
        ASSERT_NE(attrs0->find("hidden"), attrs0->end());
        el.remove_attribute("hidden");
        const auto attrs = el.attributes();
        ASSERT_EQ(attrs->find("hidden"), attrs->end());
    });
}

/* not working
TEST_F(TestUi, removeStyle) {
   test([]() {
        Gempyre::Element el(*m_ui, "styled");
        const auto style0 = el.styles({"color"}).value();
        if(style0.find("color") == style0.end() || style0.at("color") != "rgb(255, 0, 0)") {
            const auto color0 = style0.at("color");
            FAIL();
        }
        el.removeStyle("color");
        const auto style = el.styles({"color"});
        const auto color = style->at("color");
        ASSERT_NE(color, "red");
    });
}
*/

TEST_F(TestUi, setStyle) {
    test([this]() {
        Gempyre::Element el(ui(), "test-1");
        el.set_style("color", "blue");
        const auto style = el.styles({"color"});
        ASSERT_TRUE(style);
        ASSERT_TRUE(style->find("color") != style->end());
        ASSERT_EQ(style->at("color"), "rgb(0, 0, 255)");
    });
}

TEST_F(TestUi, styles) {
    test([this]() {
        Gempyre::Element el(ui(), "styled");
        const auto style0 = el.styles({"color", "font-size"}).value();
        const auto it = style0.find("color");
        ASSERT_NE(it, style0.end());
        ASSERT_EQ(style0.at("color"), "rgb(255, 0, 0)");
        ASSERT_EQ(style0.at("font-size"), "32px");
    });
}

TEST_F(TestUi, type) {
      test([this]() {
          Gempyre::Element el(ui(), "test-1");
          const auto t = el.type();
          ASSERT_TRUE(t);
          ASSERT_EQ(*t, "div");
      });
}

TEST_F(TestUi, rect) {
      test([this]() {
          Gempyre::Element el(ui(), "test-1");
          const auto r = el.rect();
          ASSERT_TRUE(r);
          ASSERT_TRUE(r->width > 0 && r->height > 0);
      });
}

TEST_F(TestUi, resource) {
    const auto r = ui().resource("/apitests.html");
    ASSERT_TRUE(r);
    const std::string html = GempyreUtils::join(*r);
    const auto p1 = html.find("html");
    const auto p2 = html.find("html");
    ASSERT_EQ(p1, p2);
}

TEST_F(TestUi, addFile) {
    const std::string test = "The quick brown fox jumps over the lazy dog";
    const auto tempFile = GempyreUtils::write_to_temp(test);
    const auto ok = ui().add_file("test_data", tempFile);
    ASSERT_TRUE(ok) << "Cannot add file " << tempFile;
    GempyreUtils::remove_file(tempFile);
    const auto r = ui().resource("test_data");
    const std::string text = GempyreUtils::join(*r);
    const auto p1 = text.find("quick");
    EXPECT_NE(p1, std::string::npos) << "Corrupted file";
    EXPECT_EQ(text.length(), test.length()) << "Mismatch file length" << text.length() << " expected:" << test.length();
}



TEST_F(TestUi, ping) {
    test([this](){
        const auto ping = ui().ping();
        ASSERT_TRUE(ping) << "No Ping!";
        ASSERT_TRUE(ping->first.count() >= 0 ) << ping->first.count();
        ASSERT_TRUE(ping->second.count() >= 0 ) << ping->second.count();
        ASSERT_TRUE(ping->first.count() < 50000) << ping->second.count();
        ASSERT_TRUE(ping->second.count() < 50000 ) << ping->second.count();
        GempyreUtils::log(GempyreUtils::LogLevel::Info, "Ping:", ping->first.count(), ping->second.count());
    });
}


TEST_F(TestUi, timerStartStop) {
    int count = 0;
    test([this, &count]() { 
        ui().after(0s, [&count](){
            ++count;
        });
        std::this_thread::sleep_for(1s);
        ui().after(0s, [&count](){
            ++count;
        });
    });
    test_wait(2s);
    EXPECT_EQ(count, 2);
}

TEST_F(TestUi, startTimer) {
    bool ok = false;
    ui().after(1000ms, [&ok, this](Gempyre::Ui::TimerId id)  {
       (void)id;
       test_exit();
       ok = true;
    });
    timeout(2s);
    ASSERT_TRUE(ok);
}

TEST_F(TestUi, startTimerNoId) {
    bool ok = false;
    ui().after(1000ms, [this, &ok]()  {
       test_exit();
       ok = true;
    });
    timeout(2s);
    ASSERT_TRUE(ok);
}

TEST_F(TestUi, stopTimer) {
    bool ok = true;
    auto id = ui().after(1000ms, [&ok, this]()  {
       ok = false;
       test_exit();
    });
    ui().after(3000ms, [this]() {
        test_exit();
       });
    ui().cancel_timer(id);
    timeout(4s);
    EXPECT_TRUE(ok);
}


TEST_F(TestUi, startManyTimers) {
    std::string t = "";
    test([&t]() {
        t += 'm';
    });
    ui().after(0ms, [&t](Gempyre::Ui::TimerId id)  {
       (void)id;
       t += 'o';
    });
    ui().after(1ms, [&t](Gempyre::Ui::TimerId id)  {
       (void)id;
       t += 'n';
    });
    ui().after(100ms, [&t](Gempyre::Ui::TimerId id)  {
       (void)id;
       t += 's';
    });
    ui().after(1000ms, [&t](Gempyre::Ui::TimerId id)  {
       (void)id;
       t += 't';
    });
    ui().after(1001ms, [&t](Gempyre::Ui::TimerId id)  {
       (void)id;
       t += 'e';
    });
    ui().after(3002ms, [&t, this](Gempyre::Ui::TimerId id)  {
       (void)id;
       t += 'r';
       test_exit();
    });
    timeout(5s);
    EXPECT_EQ("monster", t);
}

TEST_F(TestUi, timing) {
    const auto start = std::chrono::system_clock::now();
    ui().after(1000ms, [&start]()  {
        const auto end = std::chrono::system_clock::now();
        const auto diff = end - start;
        EXPECT_TRUE(diff >= 1000ms);
    });
    ui().after(2000ms, [&start]()  {
        const auto end = std::chrono::system_clock::now();
        const auto diff = end - start;
        EXPECT_TRUE(diff >= 2000ms);
    });
    ui().after(4000ms, [&start, this]()  {
        const auto end = std::chrono::system_clock::now();
        const auto diff = end - start;
        EXPECT_TRUE(diff >= 4000ms);
        test_exit();
    });
   timeout(5s);
}

TEST_F(TestUi, eval) {
        test([this]() {
            Gempyre::Element el(ui(), "styled");
            auto html = el.html();
            EXPECT_TRUE(html);
            EXPECT_NE(html.value(), "foo bear");
            ui().eval(" document.getElementById(\"styled\").innerHTML = \"foo bear\" ");
            html = el.html();
            EXPECT_TRUE(html);
            EXPECT_EQ(html.value(), "foo bear");
        });
}

