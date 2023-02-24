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



using namespace std::chrono_literals;
using namespace GempyreTest;


TEST_F(TestUi, onReload) {
    /** window.location.reload is deprecated **/
    test([](){});
    /*
    m_ui->startTimer(1s, true, [this]()  {
       m_ui->eval(R"(window.location.reload( true )); //deprecated
    )");
    });
    m_ui->startTimer(20s, true, [this]()  {
       m_ui->exit();
    });
    bool ok = false;
    m_ui->onReload([&ok, this](){
       m_ui->exit();
       ok = true;
    });
    m_ui->run();
    EXPECT_TRUE(ok);
    */
}

TEST_F(TestUi, eval) {
        m_ui->eval("document.write('<h3 id=\\\"foo\\\">Bar</h3>')");
        test([this]() {
            Gempyre::Element el(*m_ui, "foo");
            const auto html = el.html();
            EXPECT_TRUE(html);
            ASSERT_TRUE(html.value() == "Bar");
        });
}


TEST_F(TestUi, addressOf) {
    const auto htmlPage = TEST_HTML;
#ifdef HAS_FS
    ASSERT_TRUE(std::filesystem::exists(htmlPage));
#else
    ASSERT_TRUE(GempyreUtils::file_exists(htmlPage));
#endif
    test([htmlPage, this]() {
        ASSERT_TRUE(m_ui->address_of(htmlPage).length() > 0); //TODO better test would be write this as html and open it
    });
}

TEST_F(TestUi, byClass) {
    test([this]() {
        const auto classes = m_ui->by_class("test-class");
        ASSERT_TRUE(classes);
        ASSERT_EQ(classes->size(), 4); //4 test-classes
    });
}

TEST_F(TestUi, byName) {
    test([this]() {
        const auto names = m_ui->by_name("test-name");
        ASSERT_TRUE(names.has_value());
        ASSERT_EQ(names->size(), 5); //5 test-classes
    });
}

TEST_F(TestUi, devicePixelRatio) {
    test([this]() {
        const auto dr = m_ui->device_pixel_ratio();
        ASSERT_TRUE(dr);
        ASSERT_GT(*dr, 0);
    });
}

TEST_F(TestUi, ElementCreate) {
    Gempyre::Element parent(*m_ui, "test-1");
    Gempyre::Element(*m_ui, "boing", "div", parent);
    test([parent]() {
        const auto cop = parent.children();
        ASSERT_TRUE(cop.has_value());
        ASSERT_NE(std::find_if(cop->begin(), cop->end(), [](const auto el ){return el.id() == "boing";}), cop->end());
    });
}

TEST_F(TestUi, ElementCreateLater) {
    test([this]() {
        Gempyre::Element parent(*m_ui, "test-1");
        Gempyre::Element(*m_ui, "boing", "div", parent);
        const auto cop = parent.children();
        ASSERT_TRUE(cop.has_value());
        ASSERT_NE(std::find_if(cop->begin(), cop->end(), [](const auto el ){return el.id() == "boing";}), cop->end());
    });
}

TEST_F(TestUi, subscribe) {
    Gempyre::Element el(*m_ui, "test-1");
    static bool is_open = false;
    m_ui->after(1s, [&]() {
        is_open = true;
    });
    el.subscribe("test_event", [&](const Gempyre::Event& eel) {
        const bool ok = eel.element.id() == el.id();
        ASSERT_TRUE(ok);
        ASSERT_TRUE(is_open);
        m_ui->exit();
    });
    m_ui->after(2s, [&]()  {
          el.set_attribute("style", "color:green");
       });
    post_test([&]() {
        ASSERT_TRUE(is_open);
    });
    test_wait();
}

TEST_F(TestUi, setHTML) {
    Gempyre::Element el(*m_ui, "test-1");
    el.set_html("Test-dyn");
    test([el]() {
        const auto html = el.html();
        ASSERT_TRUE(html.has_value());
        ASSERT_EQ(html.value(), "Test-dyn");
    });
}

TEST_F(TestUi, setAttribute) {
    Gempyre::Element el(*m_ui, "test-1");
    el.set_attribute("value", "Test-attr-dyn");
    test([el]() {
        const auto attrs = el.attributes();
        ASSERT_TRUE(attrs.has_value());
        ASSERT_NE(attrs->find("value"), attrs->end());
        ASSERT_EQ(attrs.value().find("value")->second, "Test-attr-dyn");
    });
}

TEST_F(TestUi, attributes) {
   test([this]() {
        Gempyre::Element el(*m_ui, "test-1");
        const auto attrs = el.attributes();
        ASSERT_TRUE(attrs.has_value());
        ASSERT_NE(attrs->find("value"), attrs->end());
        ASSERT_EQ(attrs->find("value")->second, "Test-attr");
    });
}

TEST_F(TestUi, children) {
   test([this]() {
        Gempyre::Element el(*m_ui, "test-1");
        const auto chlds = el.children();
        ASSERT_TRUE(chlds.has_value());
        ASSERT_GT(chlds->size(), 2);
        ASSERT_EQ((*chlds)[2].id(), "test-child-2");
    });
}

TEST_F(TestUi, values) {
    test([this]() {
        Gempyre::Element el(*m_ui, "checkbox-1");
        const auto values = el.values();
        ASSERT_TRUE(values.has_value());
        ASSERT_NE(values->find("checked"), values->end());
        ASSERT_EQ(values->find("checked")->second, "true");
    });
}

TEST_F(TestUi, html) {
    test([this]() {
        Gempyre::Element el(*m_ui, "test-1");
        const auto html = el.html();
        ASSERT_TRUE(html.has_value());
        ASSERT_NE(html.value().find("Test-1"), std::string::npos);
    });
}

TEST_F(TestUi, remove) {
    test([this]() {
        Gempyre::Element el(*m_ui, "test-1");
        const auto chlds = el.children();
        ASSERT_TRUE(chlds.has_value());
        ASSERT_GT(chlds->size(), 3);
        ASSERT_EQ((*chlds)[3].id(), "test-child-3");
        Gempyre::Element c(*m_ui,"test-child-3");
        c.remove();
        const auto cop = el.children();
        ASSERT_TRUE(cop.has_value());
        ASSERT_EQ(std::find_if(cop->begin(), cop->end(), [](const auto el ){return el.id() == "test-child-3";}), cop->end());
    });
}

TEST_F(TestUi, removeAttribute) {
    test([this]() {
        Gempyre::Element el(*m_ui, "hidden");
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
        Gempyre::Element el(*m_ui, "test-1");
        el.set_style("color", "blue");
        const auto style = el.styles({"color"});
        ASSERT_EQ(style->at("color"), "rgb(0, 0, 255)");
    });
}

TEST_F(TestUi, styles) {
    test([this]() {
        Gempyre::Element el(*m_ui, "styled");
        const auto style0 = el.styles({"color", "font-size"}).value();
        const auto it = style0.find("color");
        ASSERT_NE(it, style0.end());
        ASSERT_EQ(style0.at("color"), "rgb(255, 0, 0)");
        ASSERT_EQ(style0.at("font-size"), "32px");
    });
}

TEST_F(TestUi, type) {
      test([this]() {
          Gempyre::Element el(*m_ui, "test-1");
          const auto t = el.type();
          ASSERT_TRUE(t);
          ASSERT_EQ(*t, "div");
      });
}

TEST_F(TestUi, rect) {
      test([this]() {
          Gempyre::Element el(*m_ui, "test-1");
          const auto r = el.rect();
          ASSERT_TRUE(r);
          ASSERT_TRUE(r->width > 0 && r->height > 0);
      });
}

int main(int argc, char **argv) {
   ::testing::InitGoogleTest(&argc, argv);
   for(int i = 1 ; i < argc; ++i)
       if(argv[i] == std::string_view("--verbose"))
            Gempyre::set_debug();
  killHeadless(); // there may be unwanted processes
  return RUN_ALL_TESTS();
}

