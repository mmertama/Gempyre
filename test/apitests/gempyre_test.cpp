#include "gempyre_test.h"
#include "gempyre_utils.h"

#include "apitests_resource.h"

using namespace GempyreTest;

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

static
std::string headlessParams(bool log = false) {
#ifdef HAS_FS
    const auto temp = std::filesystem::temp_directory_path().string();
#else
    const auto temp = GempyreUtils::path_pop(GempyreUtils::temp_name());
#endif
    return GempyreUtils::join(speed_params, " ") +  R"( --headless --remote-debugging-port=9222 --user-data-dir=)" +
#ifdef WINDOWS_OS
            GempyreUtils::substitute(temp, "/", "\\") + " --no-sandbox --disable-gpu "
#else
            temp
#endif
            + (log ? R"( --enable-logging --v=0)" : " --disable-logging ");

}

static
std::optional<std::string> systemChrome() {
    switch(GempyreUtils::current_os()) {
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

void GempyreTest::killHeadless() {
     const auto cmd =
     GempyreUtils::current_os() == GempyreUtils::OS::WinOs
        ? R"(powershell.exe -command "Get-CimInstance -ClassName Win32_Process -Filter \"CommandLine LIKE '%--headless%'\" | %{Stop-Process -Id $_.ProcessId}")"
        : "pkill -f \"(chrome)?(--headless)\"";
    const auto killStatus = std::system(cmd);
    (void) killStatus;
}


enum State : unsigned {
    WAIT = 0x1,
    TEST = 0x2
};

void TestUi::SetUp() {
    const auto chrome = systemChrome();
    m_ui = std::make_unique<Gempyre::Ui>(
                Apitests_resourceh,
                "apitests.html",
                    chrome ? chrome.value() : "",
                    headlessParams());
    m_ui->on_error([this](const auto& element, const auto& info) {
        std::cerr << element << " err:" << info;
        EXPECT_TRUE(false);
        std::exit(1);
    });
    m_ui->on_open([](){
        GEM_DEBUG("test ui on");
    });    

    const auto test_name = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    m_current_test = test_name;
    GEM_DEBUG("Setup", test_name);
    m_state = 0;
    m_postFunc = nullptr;
}

void TestUi::test_wait() {
    m_state |= WAIT;
    m_ui->run();
}


void TestUi::TearDown() {
    const auto test_name = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    assert(test_name == m_current_test);
    GEM_DEBUG("Teardown", test_name);
    if((! (m_state & WAIT)) && m_state & TEST)
        m_ui->run();
    if(m_postFunc)
        m_postFunc();
    m_postFunc = nullptr;
    m_ui.reset();
    killHeadless();    
}

void TestUi::test(const std::function<void () >& f) {
    GEM_DEBUG("after", m_current_test);
    m_state |= TEST;
    m_ui->after(0s, [f, this]() {
        GEM_DEBUG("test exit in", m_current_test);
        if(f)
            f();
        if(! (m_state & WAIT)) {
            m_ui->exit();
            GEM_DEBUG("test exit out", m_current_test);
        }
    });
}

 void TestUi::post_test(const std::function<void () >& f) {
        m_postFunc = f;
 }

