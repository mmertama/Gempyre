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

using namespace std::chrono_literals;

// collection of parameters that may speed up the chromium perf on headless
const std::vector<std::string_view> speed_params  = {
        "--disable-canvas-aa", // Disable antialiasing on 2d canvas
        "--disable-2d-canvas-clip-aa", // Disable antialiasing on 2d canvas clips
        "--disable-gl-drawing-for-tests", // BEST OPTION EVER! Disables GL drawing operations which produce pixel output. With this the GL output will not be correct but tests will run faster.
        "--disable-dev-shm-usage", // ???
        "--no-zygote", // wtf does that mean ?
        "--use-gl=swiftshader", // better cpu usage with --use-gl=desktop rather than --use-gl=swiftshader, still needs more testing.
        "--enable-webgl",
        "--hide-scrollbars",
        "--mute-audio",
        "--no-first-run",
        "--disable-infobars",
        "--disable-breakpad",
        "--window-size=1280,1024", // see defaultViewport
        "--no-sandbox", // meh but better resource comsuption
        "--disable-setuid-sandbox",
        "--ignore-certificate-errors",
        "--disable-extensions",
        "--disable-gpu",
        "--no-sandbox"
    };

std::string headlessParams(bool log = false) {
#ifdef HAS_FS
    const auto temp = std::filesystem::temp_directory_path().string();
#else
    const auto temp = GempyreUtils::pathPop(GempyreUtils::tempName());
#endif
    return GempyreUtils::join(speed_params, " ") +  R"( --headless --remote-debugging-port=9222 --user-data-dir=)" +
#ifdef WINDOWS_OS
            GempyreUtils::substitute(temp, "/", "\\") + " --no-sandbox --disable-gpu "
#else
            temp
#endif
            + (log ? R"( --enable-logging --v=0)" : " --disable-logging ");

}

std::optional<std::string> systemChrome() {
    switch(GempyreUtils::currentOS()) {
    case GempyreUtils::OS::MacOs: return R"(/Applications/Google\ Chrome.app/Contents/MacOS/Google\ Chrome)";
    case GempyreUtils::OS::WinOs: return R"(start "_" "C:\Program Files (x86)\Google\Chrome\Application\chrome.exe")";
    case GempyreUtils::OS::RaspberryOs: [[fallthrough]];
    case GempyreUtils::OS::LinuxOs: {
        auto browser = GempyreUtils::which(R"(chromium-browser)");
        if(browser)
            return *browser;
        return GempyreUtils::which(R"(google-chrome)");
        //xdg-open
    }
    default: return std::nullopt;
    }
}

void killHeadless() {
     const auto cmd =
     GempyreUtils::currentOS() == GempyreUtils::OS::WinOs
        ? R"(powershell.exe -command "Get-CimInstance -ClassName Win32_Process -Filter \"CommandLine LIKE '%--headless%'\" | %{Stop-Process -Id $_.ProcessId}")"
        : "pkill -f \"(chrome)?(--headless)\"";
    const auto killStatus = std::system(cmd);
    (void) killStatus;
}

class TestUi : public testing::Test {
    public:
    static void SetUpTestSuite() {
        const auto chrome = systemChrome();
        m_ui = std::make_unique<Gempyre::Ui>(
                    Apitests_resourceh,
                    "apitests.html",
                     chrome ? chrome.value() : "",
                     headlessParams());
        m_ui->on_error([](const auto& element, const auto& info) {
            std::cerr << element << " err:" << info;
            EXPECT_TRUE(false);
            m_ui->exit();
        });
    }

    void SetUp() override {
        const auto test_name = ::testing::UnitTest::GetInstance()->current_test_info()->name();
        m_current_test = test_name;
        GEM_DEBUG("Setup", test_name);
    }

    void TearDown() override {
        const auto test_name = ::testing::UnitTest::GetInstance()->current_test_info()->name();
        assert(test_name == m_current_test);
        GEM_DEBUG("Teardown", test_name);
        m_ui->run();
    }

    void test(const std::function<void () >& f) {
        GEM_DEBUG("after", m_current_test);
        m_ui->after(0s, [f, this]() {
            GEM_DEBUG("test exit in", m_current_test);
            m_ui->exit();
            GEM_DEBUG("test exit out", m_current_test);
        });
    }

    static void TearDownTestSuite()  {
        m_ui.reset();
        killHeadless();
    }
    static inline std::unique_ptr<Gempyre::Ui> m_ui;
    std::string m_current_test;
};



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
        test([]() {
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
    ASSERT_TRUE(GempyreUtils::fileExists(htmlPage));
#endif
    test([htmlPage]() {
        ASSERT_TRUE(m_ui->address_of(htmlPage).length() > 0); //TODO better test would be write this as html and open it
    });
}

TEST_F(TestUi, byClass) {
    test([]() {
        const auto classes = m_ui->by_class("test-class");
        ASSERT_TRUE(classes);
        ASSERT_EQ(classes->size(), 4); //4 test-classes
    });
}

TEST_F(TestUi, byName) {
    test([]() {
        const auto names = m_ui->by_name("test-name");
        ASSERT_TRUE(names.has_value());
        ASSERT_EQ(names->size(), 5); //5 test-classes
    });
}

TEST_F(TestUi, devicePixelRatio) {
    test([]() {
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
    test([]() {
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
    test([&]() {
        ASSERT_TRUE(is_open);
    });
}

TEST_F(TestUi, setHTML) {
    Gempyre::Element el(*m_ui, "test-1");
    el.set_html("Test-dyn");
    test([&]() {
        const auto html = el.html();
        ASSERT_TRUE(html.has_value());
        ASSERT_EQ(html.value(), "Test-dyn");
    });
}

TEST_F(TestUi, setAttribute) {
    Gempyre::Element el(*m_ui, "test-1");
    el.set_attribute("value", "Test-attr-dyn");
    test([&]() {
        const auto attrs = el.attributes();
        ASSERT_TRUE(attrs.has_value());
        ASSERT_NE(attrs->find("value"), attrs->end());
        ASSERT_EQ(attrs.value().find("value")->second, "Test-attr-dyn");
    });
}

TEST_F(TestUi, attributes) {
   test([]() {
        Gempyre::Element el(*m_ui, "test-1");
        const auto attrs = el.attributes();
        ASSERT_TRUE(attrs.has_value());
        ASSERT_NE(attrs->find("value"), attrs->end());
        ASSERT_EQ(attrs->find("value")->second, "Test-attr");
    });
}

TEST_F(TestUi, children) {
   test([]() {
        Gempyre::Element el(*m_ui, "test-1");
        const auto chlds = el.children();
        ASSERT_TRUE(chlds.has_value());
        ASSERT_GT(chlds->size(), 2);
        ASSERT_EQ((*chlds)[2].id(), "test-child-2");
    });
}

TEST_F(TestUi, values) {
    test([]() {
        Gempyre::Element el(*m_ui, "checkbox-1");
        const auto values = el.values();
        ASSERT_TRUE(values.has_value());
        ASSERT_NE(values->find("checked"), values->end());
        ASSERT_EQ(values->find("checked")->second, "true");
    });
}

TEST_F(TestUi, html) {
    test([]() {
        Gempyre::Element el(*m_ui, "test-1");
        const auto html = el.html();
        ASSERT_TRUE(html.has_value());
        ASSERT_NE(html.value().find("Test-1"), std::string::npos);
    });
}

TEST_F(TestUi, remove) {
    test([]() {
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
    test([]() {
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
    test([]() {
        Gempyre::Element el(*m_ui, "test-1");
        el.set_style("color", "blue");
        const auto style = el.styles({"color"});
        ASSERT_EQ(style->at("color"), "rgb(0, 0, 255)");
    });
}

TEST_F(TestUi, styles) {
    test([]() {
        Gempyre::Element el(*m_ui, "styled");
        const auto style0 = el.styles({"color", "font-size"}).value();
        const auto it = style0.find("color");
        ASSERT_NE(it, style0.end());
        ASSERT_EQ(style0.at("color"), "rgb(255, 0, 0)");
        ASSERT_EQ(style0.at("font-size"), "32px");
    });
}

TEST_F(TestUi, type) {
      test([]() {
          Gempyre::Element el(*m_ui, "test-1");
          const auto t = el.type();
          ASSERT_TRUE(t);
          ASSERT_EQ(*t, "div");
      });
}

TEST_F(TestUi, rect) {
      test([]() {
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

