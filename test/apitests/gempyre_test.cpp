#include "gempyre_test.h"
#include "gempyre_utils.h"

#include "apitests_resource.h"

#include <cstdlib>
#ifndef WINDOWS_OS
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

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
std::string headlessParams() {
    const auto debug_port = []() {
#ifndef WINDOWS_OS
        const auto socket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (socket >= 0) {
            sockaddr_in address{};
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            address.sin_port = 0;
            if (::bind(socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0) {
                socklen_t length = sizeof(address);
                if (::getsockname(socket, reinterpret_cast<sockaddr*>(&address), &length) == 0) {
                    const auto port = std::to_string(ntohs(address.sin_port));
                    ::close(socket);
                    return port;
                }
            }
            ::close(socket);
        }
#endif
        if (const auto value = std::getenv("GEMPYRE_CHROME_DEBUG_PORT"); value && *value)
            return std::string(value);
        return std::string("9222");
    }();
    const auto profile = []() {
        const auto path = GempyreUtils::temp_name();
        GempyreUtils::remove_file(path);
        return path;
    }();
    return GempyreUtils::join(common_params, " ")  + " " + GempyreUtils::join(speed_params, " ") + " --headless --remote-debugging-port=" + debug_port + " --user-data-dir=" +
#ifdef WINDOWS_OS
            GempyreUtils::substitute(profile, "/", "\\") + " --no-sandbox --disable-gpu "
#else
            profile
#endif
            + R"( --enable-logging=stderr --v=1)";

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
        for (const auto browser_name : {"google-chrome", "chrome", "chromium", "chromium-browser"}) {
            if (const auto browser = GempyreUtils::which(browser_name))
                return *browser;
        }
        return std::nullopt;
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
    GempyreUtils::log(GempyreUtils::LogLevel::Info, "killHeadles", killStatus);
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
        GempyreUtils::log(GempyreUtils::LogLevel::Info, "Chrome executable", *chrome);
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
            GempyreUtils::log(GempyreUtils::LogLevel::Error, "Chrome not started");
            FAIL() << "Chrome not started!" << " ui:" << (m_ui->ui_available() ? "ok" : "nok")  << " t:" << (m_ui->is_timer_on_hold() ? "ok" : "nok");
            std::exit(2);
            });
        m_ui->on_open([wait_start]() {
            PRINT_D("test ui on");
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


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    for(int i = 1 ; i < argc; ++i) {
       if(argv[i] == std::string_view("--verbose"))
            Gempyre::set_debug();
    }   
    killHeadless(); // there may be unwanted processes
    const auto exit_code =  RUN_ALL_TESTS();
    killHeadless();
    return exit_code;
}
