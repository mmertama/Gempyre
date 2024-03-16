#include "gempyre_test.h"
#include "gempyre_utils.h"

#include "apitests_resource.h"

using namespace GempyreTest;

#define FAST

// collection of parameters that may speed up the chromium perf on headless
const std::vector<std::string_view> speed_params  = {
        "--disable-canvas-aa", // Disable antialiasing on 2d canvas
        "--disable-2d-canvas-clip-aa", // Disable antialiasing on 2d canvas clips
        "--disable-gl-drawing-for-tests", // BEST OPTION EVER! Disables GL drawing operations which produce pixel output. With this the GL output will not be correct but tests will run faster.
        "--disable-dev-shm-usage", // ???
        "--no-zygote", // wtf does that mean ?
        "--use-gl=swiftshader", // better cpu usage with --use-gl=desktop rather than --use-gl=swiftshader, still needs more testing.
        "--enable-webgl"
    };

const std::vector<std::string_view> common_params  = {
    "--window-size=1280,1024", // see defaultViewport
        "--no-sandbox", // meh but better resource comsuption
        "--disable-setuid-sandbox",
        "--ignore-certificate-errors",
        "--disable-extensions",
        "--disable-gpu",
        "--no-sandbox",
        "--disable-software-rasterizer",
        "--disable-features=DefaultPassthroughCommandDecoder",
        "--disable-extensions",
        "--disable-translate",
        "--disable-sync",
        "--use-angle=swiftshader",
        "--hide-scrollbars",
        "--mute-audio",
        "--no-first-run",
        "--disable-infobars",
        "--disable-breakpad"
};

static
std::string headlessParams(bool log = false) {
#ifdef HAS_FS
    const auto temp = std::filesystem::temp_directory_path().string();
#else
    const auto temp = GempyreUtils::path_pop(GempyreUtils::temp_name());
#endif
    return GempyreUtils::join(common_params, " ")  + " " + GempyreUtils::join(speed_params, " ") +  R"( --headless --remote-debugging-port=9222 --user-data-dir=)" +
#ifdef WINDOWS_OS
            GempyreUtils::substitute(temp, "/", "\\") + " --no-sandbox --disable-gpu "
#else
            temp
#endif
            + (log ? R"( --enable-logging --v=0 )" :  R"( --disable-logging --log-level=3 --log-path=/dev/null )");

}

static
std::optional<std::string> systemChrome() {
    switch(GempyreUtils::current_os()) {
    case GempyreUtils::OS::MacOs: return R"(/Applications/Google\ Chrome.app/Contents/MacOS/Google\ Chrome)";
    case GempyreUtils::OS::WinOs: {
            auto browser = GempyreUtils::which(R"(chrome)");
            return std::string(R"(start "_" )") + (browser ? GempyreUtils::qq(*browser) : 
            std::string(R"("C:\Program Files (x86)\Google\Chrome\Application\chrome.exe")"));
    }
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

TestUi::TestUi() {
}

TestUi::~TestUi() {
}

void TestUi::SetUpTestSuite() {
}

void TestUi::TearDownTestSuite() {
    if(m_ui) {
        m_ui->after(0s, [](){
            m_ui->exit();
        });
        m_ui->resume();
    }
    m_ui.reset();
}

void TestUi::SetUp() {
    if(!m_ui) {
        const auto chrome = systemChrome();
        if(!chrome) {
            GempyreUtils::log(GempyreUtils::LogLevel::Error, "Chrome not found!");
            FAIL() <<"Chrome not found!";
            std::exit(1);
        }
        m_ui = std::make_unique<Gempyre::Ui>(
                    Apitests_resourceh,
                    "apitests.html",
                    *chrome,
                    headlessParams());
        m_ui->on_error([](const auto& element, const auto& info) {
            GempyreUtils::log(GempyreUtils::LogLevel::Error, element, "err:", info);
            EXPECT_TRUE(false);
            std::exit(1);
        });
        const auto wait_start = GempyreUtils::wait_expire(30s, []() {
            GempyreUtils::log(GempyreUtils::LogLevel::Error, "Chorome not start");
            FAIL() << "Chrome not start!";
            std::exit(2);
            });
        m_ui->on_open([wait_start](){
            GEM_DEBUG("test ui on");
        });
#ifdef FAST
        m_ui->after(0ms, []() {
            m_ui->suspend();
        });
        m_ui->run();
#endif
    }

    const auto test_name = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    m_current_test = test_name;
    m_state = 0;
    m_postFunc = nullptr;
}

void TestUi::exit() {
#ifdef FAST
    m_ui->suspend();
#else
    m_ui->exit();
#endif
}

void TestUi::run() {
 #ifdef FAST
    m_ui->resume();
 #else   
    m_ui->run();
#endif
}

void TestUi::finish() {
#ifndef FAST
    m_ui.reset();
    killHeadless(); 
#endif     
}

void TestUi::test_wait(std::chrono::milliseconds wait) {
    m_state |= WAIT;
    m_ui->after(wait, [this]() {
        exit();
    });
    run();
}



std::chrono::milliseconds TestUi::timeout(std::chrono::milliseconds wait) {
    const auto start = std::chrono::high_resolution_clock::now();
    [[maybe_unused]] const auto test_name = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    m_state |= WAIT;
    ui().after(wait, [this, wait]([[maybe_unused]] const auto tid) {
        exit();
        // FAIL do return this function
        FAIL() << "Timeout in " << m_current_test << " waited: " << wait.count() << "ms ";
    });
    run();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>
    (std::chrono::duration<double, std::milli>(end - start));
}


void TestUi::TearDown() {
    [[maybe_unused]] const auto test_name = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    assert(test_name == m_current_test);
    if((! (m_state & WAIT)) && m_state & TEST) {
        run();
    }
    if(m_postFunc)
        m_postFunc();
    m_postFunc = nullptr;
    finish();
}

void TestUi::test(const std::function<void () >& f) {
    m_state |= TEST;
    m_ui->after(0s, [f, this]() {
        if(f)
            f();
        if(! (m_state & WAIT)) {
            exit();
        }
    });
}

 void TestUi::post_test(const std::function<void () >& f) {
        m_postFunc = f;
 }

void TestUi::test_exit() {
    exit();
}

Gempyre::Ui& TestUi::ui() {
    return *m_ui;
}

std::string_view TestUi::current_test() const {
    return m_current_test;
}

class StderrListener : public ::testing::EmptyTestEventListener {
public:
    void OnTestPartResult(const ::testing::TestPartResult& test_part_result) override {
        // Check if the message contains the error pattern
        if (test_part_result.type() == ::testing::TestPartResult::kNonFatalFailure &&
            strstr(test_part_result.message(), "Skipping mandatory platform policies") != nullptr) {
            // Skip the test
            GTEST_SKIP();
        }
    }
};

int main(int argc, char **argv) {
   ::testing::InitGoogleTest(&argc, argv);
   for(int i = 1 ; i < argc; ++i)
       if(argv[i] == std::string_view("--verbose"))
            Gempyre::set_debug();       
  killHeadless(); // there may be unwanted processes
  ::testing::UnitTest::GetInstance()->listeners().Append(new StderrListener);
  return RUN_ALL_TESTS();
}
