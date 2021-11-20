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

std::string defaultChrome() {
    switch(GempyreUtils::currentOS()) {
    case GempyreUtils::OS::MacOs: return R"(/Applications/Google\ Chrome.app/Contents/MacOS/Google\ Chrome)";
    case GempyreUtils::OS::WinOs: return R"(start "_" "C:\Program Files (x86)\Google\Chrome\Application\chrome.exe")";
    case GempyreUtils::OS::LinuxOs: {
        auto browser = GempyreUtils::which(R"(chromium-browser)");
        if(!browser.empty())
            return browser;
        return GempyreUtils::which(R"(google-chrome)");
        //xdg-open
    }
    default: return "";
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
    protected:
    void SetUp() override {
        m_ui = std::make_unique<Gempyre::Ui>(
                    Apitests_resourceh,
                    "apitests.html",
                     defaultChrome(),
                     headlessParams());
        m_ui->onError([this](const auto& element, const auto& info) {
            std::cerr << element << " err:" << info;
            EXPECT_TRUE(false);
            m_ui->exit();
        });
    }
    void TearDown() override {
        m_ui.reset();
        killHeadless();
    }
    std::unique_ptr<Gempyre::Ui> m_ui;
};

/*
#define CONSTRUCT_UI Gempyre::Ui ui({{"/test.html", Unittestshtml}}, "test.html", browser);
*/
/*
class Waiter {
    Waiter() {}
};
*/

#define TEST_FAIL (assert(false))

TEST(UiTests, openPage_with_page_browser) {
    constexpr auto htmlPage = TEST_HTML;
#ifdef HAS_FS
    ASSERT_TRUE(std::filesystem::exists(htmlPage));
#else
    ASSERT_TRUE(GempyreUtils::fileExists(htmlPage));
#endif
    const auto browser = defaultChrome();

    Gempyre::Ui::Filemap map;
    const auto url = Gempyre::Ui::addFile(map, htmlPage);

    gempyre_utils_assert_x(url, std::string("Cannot load ") + htmlPage);

    Gempyre::Ui ui(map, *url, defaultChrome(), headlessParams(), 50000);
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
    killHeadless();
    ASSERT_TRUE(ok);
}


TEST(UiTests, openPage_with_page) {
    constexpr auto htmlPage = TEST_HTML;
#ifdef HAS_FS
    ASSERT_TRUE(std::filesystem::exists(htmlPage));
#else
    ASSERT_TRUE(GempyreUtils::fileExists(htmlPage));
#endif
    const auto browser = defaultChrome();

    Gempyre::Ui::Filemap map;
    const auto url = Gempyre::Ui::addFile(map, htmlPage);

    gempyre_utils_assert_x(url, std::string("Cannot load ") + htmlPage);
    Gempyre::Ui ui(map, *url, defaultChrome(), headlessParams());
    bool ok = false;
    ui.onOpen([&ui, &ok]() {
        ok = true;
        ui.exit();
    });
    ui.onError([](const auto& element, const auto& info){std::cerr << element << " err:" << info; TEST_FAIL;});
    const auto raii_ex = GempyreUtils::waitExpire(10s, []() {ASSERT_TRUE(false);});
    ui.run();
    killHeadless();
    ASSERT_TRUE(ok);
}


TEST(UiTests, openPage_with_browser) {
    const auto browser = defaultChrome();
    Gempyre::Ui ui({{"/foobar.html", Apitestshtml}}, "foobar.html", browser, headlessParams(), 50000);
    bool ok = false;
    ui.onOpen([&ui, &ok]() {
        ok = true;
        ui.exit();
    });
    ui.onError([](const auto& element, const auto& info){std::cerr << element << " err:" << info; TEST_FAIL;});
    const auto raii_ex = GempyreUtils::waitExpire(4s, []() {ASSERT_TRUE(false);});
    ui.run();
    killHeadless();
    ASSERT_TRUE(ok);

}

TEST(UiTests, openPage_defaults) {
    Gempyre::Ui ui(Apitests_resourceh, "apitests.html", defaultChrome(), headlessParams());
    bool ok = false;
    ui.onOpen([&ui, &ok]() {
        ok = true;
        ui.exit();
    });
    ui.onError([](const auto& element, const auto& info){std::cerr << element << " err:" << info; TEST_FAIL;});
    const auto raii_ex = GempyreUtils::waitExpire(4s, []() {TEST_FAIL;});
    ui.run();
    killHeadless();
    ASSERT_TRUE(ok);
}

TEST_F(TestUi, exit) {
    bool ok = false;
    m_ui->onOpen([&ok, this](){
       ok = true;
       m_ui->exit();
    });
    m_ui->run();
    EXPECT_TRUE(ok);
}

TEST_F(TestUi, exit_on_time) {
    bool ok = false;
    m_ui->after(2s, [this, &ok]()  {
       m_ui->exit();
       ok = true;
    });
    m_ui->run();
    EXPECT_TRUE(ok);
}


TEST_F(TestUi, onExit) {
    m_ui->after(2s, [this]() {
       m_ui->close();
    });
    bool ok = false;
    m_ui->onExit([&ok, this](){
        ok = true;
        m_ui->exit();
    });
    m_ui->run();
    EXPECT_TRUE(ok);
}

TEST_F(TestUi, close) {
    bool ok = false;
    m_ui->onOpen([&ok, this](){
       ok = true;
       m_ui->close();
    });
    m_ui->after(2s, [this]()  {
      m_ui->exit();
    });
    m_ui->run();
    EXPECT_TRUE(ok);
}

TEST_F(TestUi, onReload) {
    /** window.location.reload is deprecated **/
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

TEST_F(TestUi, onOpen){
    bool ok = false;
    m_ui->onOpen([&ok, this](){
      ok = true;
      m_ui->exit();
    });
    m_ui->run();
    EXPECT_TRUE(ok);
}

TEST_F(TestUi, run) {
    bool ok = false;
    m_ui->onOpen([&ok, this](){
        m_ui->after(1000ms, [this, &ok]()  {
           m_ui->exit();
           ok = true;
        });
    });
    m_ui->onError([](const auto& element, const auto& info){std::cerr << element << " err:" << info; TEST_FAIL;});
    m_ui->run();
    EXPECT_TRUE(ok);
}

TEST_F(TestUi, setLogging) {
    m_ui->after(3s, [this]()  {
       m_ui->setLogging(false);
       m_ui->exit();
    });
    m_ui->onError([](const auto& element, const auto& info){std::cerr << element << " err:" << info; TEST_FAIL;});
    m_ui->setLogging(true); //how to test? TODO get log from UI
    m_ui->run();
}


TEST_F(TestUi, eval) {
        m_ui->eval("document.write('<h3 id=\\\"foo\\\">Bar</h3>')");
        bool ok = false;
        m_ui->onOpen([&ok, this]() {
            Gempyre::Element el(*m_ui, "foo");
            const auto html = el.html();
            ASSERT_TRUE(html);
            ok = html.value() == "Bar";
            m_ui->exit();
        });
        m_ui->run();
        EXPECT_TRUE(ok);
}

TEST_F(TestUi, debug) {
    m_ui->after(1000ms, [this]()  {
       m_ui->exit();
    });
    m_ui->onError([](const auto& element, const auto& info){std::cerr << element << " err:" << info; TEST_FAIL;});
    m_ui->debug("Test - Debug");
    m_ui->run();
}

TEST_F(TestUi, alert) {
 /*
  * Requires user interaction, skip
  *
    m_ui->startTimer(1000ms, true, [this]()  {
       m_ui->exit();
    });
    m_ui->onError([](const auto& element, const auto& info){std::cerr << element << " err:" << info; TEST_FAIL;});
    m_ui->alert("Test - Alert");
    m_ui->run();*/
}


TEST_F(TestUi, open) {
    m_ui->after(1000ms, [this]()  {
       m_ui->exit();
    });
    m_ui->onError([](const auto& element, const auto& info){std::cerr << element << " err:" << info; TEST_FAIL;});
    m_ui->open("http://www.google.com");
    m_ui->run();
}

TEST_F(TestUi, startTimer) {
    m_ui->after(1000ms, [this](Gempyre::Ui::TimerId id)  {
       (void)id;
       m_ui->exit();
    });
    m_ui->run();
}

TEST_F(TestUi, startTimerNoId) {
    m_ui->after(1000ms, [this]()  {
       m_ui->exit();
    });
    m_ui->run();
}

TEST_F(TestUi, stopTimer) {
    bool ok = true;
    auto id = m_ui->after(1000ms, [this, &ok]()  {
       ok = false;
       m_ui->exit();
    });
    m_ui->after(3000ms, [this]() {
          m_ui->exit();
       });
    m_ui->cancel(id);
    m_ui->run();
    EXPECT_TRUE(ok);
}


TEST_F(TestUi, startManyTimers) {
    std::string test = "";
    m_ui->onOpen([&test](){
        test += 'm';
    });
    m_ui->after(0ms, [&test](Gempyre::Ui::TimerId id)  {
       (void)id;
       test += 'o';
    });
    m_ui->after(1ms, [&test](Gempyre::Ui::TimerId id)  {
       (void)id;
       test += 'n';
    });
    m_ui->after(100ms, [&test](Gempyre::Ui::TimerId id)  {
       (void)id;
       test += 's';
    });
    m_ui->after(1000ms, [&test](Gempyre::Ui::TimerId id)  {
       (void)id;
       test += 't';
    });
    m_ui->after(1001ms, [&test](Gempyre::Ui::TimerId id)  {
       (void)id;
       test += 'e';
    });
    m_ui->after(10002ms, [&test, this](Gempyre::Ui::TimerId id)  {
       (void)id;
       test += 'r';
       m_ui->exit();
    });
    m_ui->run();
    EXPECT_EQ("monster", test);
}

TEST_F(TestUi, timing) {
    const auto start = std::chrono::system_clock::now();
    m_ui->after(1000ms, [&start]()  {
        const auto end = std::chrono::system_clock::now();
        const auto diff = end - start;
        EXPECT_TRUE(diff >= 1000ms);
    });
    m_ui->after(2000ms, [&start]()  {
        const auto end = std::chrono::system_clock::now();
        const auto diff = end - start;
        EXPECT_TRUE(diff >= 2000ms);
    });
    m_ui->after(4000ms, [&start, this]()  {
        const auto end = std::chrono::system_clock::now();
        const auto diff = end - start;
        EXPECT_TRUE(diff >= 4000ms);
        m_ui->exit();
    });
   m_ui->run();
}

TEST_F(TestUi, root) {
    EXPECT_EQ(m_ui->root().id(), ""); //root has no id
}

TEST_F(TestUi, addressOf) {
    const auto htmlPage = TEST_HTML;
#ifdef HAS_FS
    ASSERT_TRUE(std::filesystem::exists(htmlPage));
#else
    ASSERT_TRUE(GempyreUtils::fileExists(htmlPage));
#endif
    m_ui->onOpen([this, htmlPage](){
        EXPECT_TRUE(m_ui->addressOf(htmlPage).length() > 0); //TODO better test would be write this as html and open it
        m_ui->exit();
    });
}

TEST_F(TestUi, byClass) {
    m_ui->onOpen([this]() {
        const auto classes = m_ui->byClass("test-class");
        ASSERT_TRUE(classes);
        EXPECT_EQ(classes->size(), 4); //4 test-classes
        m_ui->exit();
    });
    m_ui->run();
}

TEST_F(TestUi, byName) {
    bool ok = false;
    m_ui->onOpen([&ok, this](){
        const auto names = m_ui->byName("test-name");
        ok = names.has_value() && names->size() == 5; //5 test-classes
        m_ui->exit();
    });
    m_ui->run();
    EXPECT_TRUE(ok);
}

#if !defined(CI_ACTIONS) // Actions fails with these time limits (sometimes), we skip

TEST_F(TestUi, ping) {
    bool ok = false;
    m_ui->onOpen([&ok, this](){
        const auto ping = m_ui->ping();
        ok = ping.has_value() &&
         ping->first.count() > 0 &&
         ping->second.count() > 0 &&
         ping->first.count() < 20000 &&
         ping->second.count() < 20000;
        if(ping)
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Ping:", ping->first.count(), ping->second.count());
        else
             GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Ping: N/A");
        m_ui->exit();
    });
    m_ui->run();
    EXPECT_TRUE(ok);
}

#endif

TEST_F(TestUi, resource) {
    const auto r = m_ui->resource("/apitests.html");
    ASSERT_TRUE(r);
    const std::string html = GempyreUtils::join(*r);
    const auto p1 = html.find("html");
    const auto p2 = html.find("html");
    ASSERT_EQ(p1, p2);
}

TEST_F(TestUi, addFile) {
    const std::string test = "The quick brown fox jumps over the lazy dog";
    const auto tempFile = GempyreUtils::writeToTemp(test);
    const auto ok = m_ui->addFile("test_data", tempFile);
    ASSERT_TRUE(ok);
    GempyreUtils::removeFile(tempFile);
    const auto r = m_ui->resource("test_data");
    const std::string text = GempyreUtils::join(*r);
    const auto p1 = text.find("quick");
    EXPECT_NE(p1, std::string::npos);
    EXPECT_EQ(text.length(), test.length());
}

TEST_F(TestUi, devicePixelRatio) {
    m_ui->onOpen([this](){
        const auto dr = m_ui->devicePixelRatio();
        ASSERT_TRUE(dr);
        EXPECT_TRUE(*dr > 0);
        m_ui->exit();
    });
    m_ui->run();
}

TEST_F(TestUi, ElementCreate) {
    Gempyre::Element parent(*m_ui, "test-1");
    Gempyre::Element(*m_ui, "boing", "div", parent);
    bool ok = false;
    m_ui->onOpen([&ok, this, &parent]() {
        const auto cop = parent.children();
        ok = cop.has_value() && std::find_if(cop->begin(), cop->end(), [](const auto el ){return el.id() == "boing";}) != cop->end();
        m_ui->exit();
    });
    m_ui->run();
    ASSERT_TRUE(ok);
}

TEST_F(TestUi, ElementCreateLater) {
    bool ok = false;
    m_ui->onOpen([&ok, this]() {
        Gempyre::Element parent(*m_ui, "test-1");
        Gempyre::Element(*m_ui, "boing", "div", parent);
        const auto cop = parent.children();
        ok = cop.has_value() && std::find_if(cop->begin(), cop->end(), [](const auto el ){return el.id() == "boing";}) != cop->end();
        m_ui->exit();
    });
    m_ui->run();
    ASSERT_TRUE(ok);
}

TEST_F(TestUi, idTest) {
    Gempyre::Element foo(*m_ui, "test-1");
    ASSERT_EQ(foo.id(), "test-1");
}


TEST_F(TestUi, subscribe) {
    bool ok = false;
    Gempyre::Element el(*m_ui, "test-1");
    bool is_open = false;
    m_ui->onOpen([&is_open]() {
        is_open = true;
    });
    el.subscribe("test_event", [&](const Gempyre::Event& eel) {
        ok = eel.element.id() == el.id();
        ASSERT_TRUE(ok);
        ASSERT_TRUE(is_open);
        m_ui->exit();
    });
    m_ui->after(2s, [&]()  {
          el.setAttribute("style", "color:green");
       });
    m_ui->after(10s, [&]()  {
          m_ui->exit();
          ASSERT_TRUE(is_open);
       });
    m_ui->run();
    ASSERT_TRUE(is_open);
    ASSERT_TRUE(ok);
}

TEST_F(TestUi, setHTML) {
    Gempyre::Element el(*m_ui, "test-1");
    el.setHTML("Test-dyn");
    bool ok = false;
    m_ui->onOpen([&ok, this, &el]() {
        const auto html = el.html();
        ok = html.has_value() && html.value() == "Test-dyn";
        m_ui->exit();
    });
    m_ui->run();
    ASSERT_TRUE(ok);
}

TEST_F(TestUi, setAttribute) {
    Gempyre::Element el(*m_ui, "test-1");
    el.setAttribute("value", "Test-attr-dyn");
    bool ok = false;
    m_ui->onOpen([&ok, this, &el]() {
        const auto attrs = el.attributes();
        ok = attrs.has_value()
                && attrs->find("value") != attrs->end()
                && attrs.value().find("value")->second == "Test-attr-dyn";
        m_ui->exit();
    });
    m_ui->run();
    ASSERT_TRUE(ok);
}

TEST_F(TestUi, attributes) {
    bool ok = false;
    m_ui->onOpen([&ok, this]() {
        Gempyre::Element el(*m_ui, "test-1");
        const auto attrs = el.attributes();
        ok = attrs.has_value()
                && attrs->find("value") != attrs->end()
                && attrs->find("value")->second == "Test-attr";
        m_ui->exit();
    });
    m_ui->run();
    ASSERT_TRUE(ok);
}

TEST_F(TestUi, children) {
    bool ok = false;
    m_ui->onOpen([&ok, this]() {
        Gempyre::Element el(*m_ui, "test-1");
        const auto chlds = el.children();
        ok = chlds.has_value()
                && chlds->size() > 2
                && (*chlds)[2].id() == "test-child-2";
        m_ui->exit();
    });
    m_ui->run();
    ASSERT_TRUE(ok);
}

TEST_F(TestUi, values) {
    bool ok = false;
    m_ui->onOpen([&ok, this]() {
        Gempyre::Element el(*m_ui, "checkbox-1");
        const auto values = el.values();
        ok = values.has_value()
                && values->find("checked") != values->end()
                && values->find("checked")->second == "true";
        m_ui->exit();
    });
    m_ui->run();
    ASSERT_TRUE(ok);
}

TEST_F(TestUi, html) {
    bool ok = false;
    m_ui->onOpen([&ok, this]() {
        Gempyre::Element el(*m_ui, "test-1");
        const auto html = el.html();
        ok = html.has_value() && html.value().find("Test-1") != std::string::npos;
        m_ui->exit();
    });
    m_ui->run();
    ASSERT_TRUE(ok);
}

TEST_F(TestUi, remove) {
    bool ok = false;
    m_ui->onOpen([&ok, this]() {
        Gempyre::Element el(*m_ui, "test-1");
        const auto chlds = el.children();
        ok = chlds.has_value()
                && chlds->size() > 3
                && (*chlds)[3].id() == "test-child-3";
        ASSERT_TRUE(ok);
        Gempyre::Element c(*m_ui,"test-child-3");
        c.remove();
        const auto cop = el.children();
        ok = cop.has_value() && std::find_if(cop->begin(), cop->end(), [](const auto el ){return el.id() == "test-child-3";}) == cop->end();
        m_ui->exit();
    });
    m_ui->run();
    ASSERT_TRUE(ok);
}

TEST_F(TestUi, removeAttribute) {
    bool ok = false;
    m_ui->onOpen([&ok, this]() {
        Gempyre::Element el(*m_ui, "hidden");
        auto attrs0 = el.attributes();
        ASSERT_NE(attrs0->find("hidden"), attrs0->end());
        el.removeAttribute("hidden");
        const auto attrs = el.attributes();
        ok = attrs->find("hidden") == attrs->end();
        m_ui->exit();
    });
    m_ui->run();
    ASSERT_TRUE(ok);
}

TEST_F(TestUi, removeStyle) {
    bool ok = false;
    m_ui->onOpen([&ok, this]() {
        Gempyre::Element el(*m_ui, "styled");
        const auto style0 = el.styles({"color"}).value();
        if(style0.find("color") == style0.end() || style0.at("color") != "rgb(255, 0, 0)") {
            const auto color0 = style0.at("color");
            m_ui->exit();
            return;
        }
        el.removeStyle("color");
        const auto style = el.styles({"color"});
        const auto color = style->at("color");
        ok = color != "red";
        m_ui->exit();
    });
    m_ui->run();
    ASSERT_TRUE(ok);
}

TEST_F(TestUi, setStyle) {
    bool ok = false;
    m_ui->onOpen([&ok, this]() {
        Gempyre::Element el(*m_ui, "test-1");
        el.setStyle("color", "blue");
        const auto style = el.styles({"color"});
        ok = style->at("color") == "rgb(0, 0, 255)";
        m_ui->exit();
    });
    m_ui->run();
    ASSERT_TRUE(ok);
}

TEST_F(TestUi, styles) {
    bool ok = false;
    m_ui->onOpen([&ok, this]() {
        Gempyre::Element el(*m_ui, "styled");
        const auto style0 = el.styles({"color", "font-size"}).value();
        const auto it = style0.find("color");
        ok = it != style0.end() && style0.at("color") == "rgb(255, 0, 0)" && style0.at("font-size") == "32px";
        m_ui->exit();
    });
    m_ui->run();
    ASSERT_TRUE(ok);
}

TEST_F(TestUi, type) {
      m_ui->onOpen([this]() {
          Gempyre::Element el(*m_ui, "test-1");
          const auto t = el.type();
          ASSERT_TRUE(t);
          ASSERT_EQ(*t, "div");
          m_ui->exit();
      });
      m_ui->run();
}

TEST_F(TestUi, rect) {
      m_ui->onOpen([this]() {
          Gempyre::Element el(*m_ui, "test-1");
          const auto r = el.rect();
          ASSERT_TRUE(r);
          EXPECT_TRUE(r->width > 0 && r->height > 0);
          m_ui->exit();
      });
      m_ui->run();
}

int main(int argc, char **argv) {
   ::testing::InitGoogleTest(&argc, argv);
   for(int i = 1 ; i < argc; ++i)
       if(argv[i] == std::string_view("--verbose"))
            Gempyre::setDebug();
  killHeadless(); // there may be unwanted processes
  return RUN_ALL_TESTS();
}

