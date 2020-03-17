#include <telex.h>
#include <telex_utils.h>
#include <iostream>
#include <vector>
#include <tuple>
#include <functional>
#include <set>
#include "unittests_resource.h"

using namespace std::chrono_literals;

#define CONSTRUCT_UI Telex::Ui ui({{"/test.html", Unittestshtml}}, "test.html", browser);

std::string headlessParams() {
    switch(TelexUtils::currentOS()) {
    case TelexUtils::OS::WIN: return  "--headless --disable-gpu --remote-debugging-port=9222";
    default:return "--headless --disable-gpu";
    }
}


std::string defaultChrome() {
    switch(TelexUtils::currentOS()) {
    case TelexUtils::OS::MAC: return R"(/Applications/Google\ Chrome.app/Contents/MacOS/Google\ Chrome)";
    case TelexUtils::OS::WIN: return  R"("C:\Program Files (86)\Google\Chrome\Application\chrome.exe")";
    case TelexUtils::OS::LINUX: return  R"(chromium-browser)";  //R"(google-chrome)";
    default: return "";
    }
}

class Waiter {
    Waiter() {}
};


const std::vector<std::tuple<std::string, std::function<bool (const std::string&)>>>Uitests = {

{R"(explicit Ui(const std::string& indexHtml, const std::string& browser, const std::string& extraParams, unsigned short port))",
        [](const std::string& ){
    const std::string html = TelexUtils::systemEnv("TELEX-UNITTEST-HTML") ;
    if(html.empty()) {
        std::cerr << "no TELEX-UNITTEST-HTML set" << std::endl;
        return false;
    }

    const std::string browser = TelexUtils::systemEnv("TELEX-UNITTEST-HTML") ;
    if(browser.empty()) {
        std::cerr << "no TELEX-UNITTEST-BROWSER set" << std::endl;
        return false;
    }
    Telex::Ui ui(html, browser, "", 50000);
    ui.exit();
    ui.run();
    return true;
}},
{R"(explicit Ui(const std::string& indexHtml))",
        [](const std::string& ){
    const std::string html = TelexUtils::systemEnv("TELEX-UNITTEST-HTML") ;
    if(html.empty()) {
        std::cerr << "no TELEX-UNITTEST-HTML set" << std::endl;
        return false;
    }
    Telex::Ui ui(html);
    ui.exit();
    ui.run();
    return true;
}},
{R"(explicit Ui(const Filemap& filemap, const std::string& indexHtml, const std::string& browser, const std::string& extraParams, unsigned short port))",
        [](const std::string& ){
    const std::string browser = TelexUtils::systemEnv("TELEX-UNITTEST-HTML") ;
    if(browser.empty()) {
        std::cerr << "no TELEX-UNITTEST-BROWSER set" << std::endl;
        return false;
    }
    Telex::Ui ui({{"/test.html", Unittestshtml}}, "test.html", browser, "", 50000);
    return true;
}},
{R"(explicit Ui(const Filemap& filemap, const std::string& indexHtml, unsigned short port))",
        [](const std::string& browser){
    CONSTRUCT_UI
    return true;
}},
{R"(exit on time))",
        [](const std::string& browser){
    CONSTRUCT_UI
    bool ok = false;
    ui.startTimer(1000ms, true, [&ui, &ok]()  {
       ui.exit();
       ok = true;
    });
    ui.run();
    return ok;
}},
{R"(void close())",
        [](const std::string& browser){
    CONSTRUCT_UI
    bool ok = false;
    ui.onOpen([&ok, &ui](){
       ok = true;
       ui.close();
    });
    ui.startTimer(2s, true, [&ui]()  {
       ui.exit();
    });
    ui.run();
    return ok;
}},
{R"(Ui& onExit(std::function<void ()> onExitFunction = nullptr))",
        [](const std::string& browser) {
    CONSTRUCT_UI
    ui.startTimer(10s, true, [&ui]() {
       ui.close();
    });
    bool ok = false;
    ui.onExit([&ok, &ui](){
        ok = true;
        ui.exit();
    });
    ui.run();
    return ok;
}},
{R"(Ui& onReload(std::function<void ()> onExitFunction = nullptr))",
        [](const std::string& browser){
    CONSTRUCT_UI
    ui.startTimer(1000ms, true, [&ui]()  {
       ui.eval("window.location.reload(false);");
    });
    ui.startTimer(3000ms, true, [&ui]()  {
       ui.exit();
    });
    bool ok = false;
    ui.onReload([&ok, &ui](){
       ui.exit();
       ok = true;
    });
    ui.run();
    return ok;
}},
{R"(Ui& onOpen(std::function<void ()> onOpenFunction = nullptr))",
        [](const std::string& browser){
    CONSTRUCT_UI
    bool ok = false;
    ui.onOpen([&ok, &ui](){
      ok = true;
      ui.exit();
    });
    ui.run();
    return ok;
}},
{R"(void run())",
        [](const std::string& browser){
    CONSTRUCT_UI
    bool ok = false;
    ui.onOpen([&ok, &ui](){
        ui.startTimer(1000ms, true, [&ui, &ok]()  {
           ui.exit();
           ok = true;
        });
    });
    ui.run();
    return ok;
}},
{R"(void setLogging(bool logging))",
        [](const std::string& browser){
    CONSTRUCT_UI
    ui.startTimer(1000ms, true, [&ui]()  {
       ui.exit();
    });
    ui.setLogging(false); //how to test?
    ui.run();
    return true;
}},
{R"(void eval(const std::string& eval))",
        [](const std::string& browser){
        CONSTRUCT_UI
        ui.eval("document.write('<h3 id=\\\"foo\\\">Bar</h3>')");
        bool ok = false;
        ui.onOpen([&ok, &ui]() {
            Telex::Element el(ui, "foo");
            const auto html = el.html();
            ok = html.has_value() && html.value() == "Bar";
            ui.exit();
        });
        ui.run();
        return ok;
}},
{R"(void debug(const std::string& msg))", [](const std::string& browser) {
    CONSTRUCT_UI
    ui.startTimer(1000ms, true, [&ui]()  {
       ui.exit();
    });
    ui.debug("Test - Debug");
    ui.run();
    return true;
}},
{R"(void alert(const std::string& msg))", [](const std::string& browser) {
    CONSTRUCT_UI
    ui.startTimer(1000ms, true, [&ui]()  {
       ui.exit();
    });
    ui.alert("Test - Alert");
    ui.run();
    return true;
}},
{R"(void open(const std::string& url, const std::string& name = ""))",
        [](const std::string& browser){
        CONSTRUCT_UI
        ui.startTimer(1000ms, true, [&ui]()  {
           ui.exit();
        });
        ui.open("http://www.google.com");
        ui.run();
        return true;
}},
{R"(TimerId startTimer(const std::chrono::milliseconds& ms, const std::function<bool (TimerId id)>& timerFunc))",
        [](const std::string& browser) {
        CONSTRUCT_UI
        ui.startTimer(1000ms, true, [&ui](Telex::Ui::TimerId /*id*/)  {
           ui.exit();
        });
        ui.run();
        return true;
}},
{R"(TimerId startTimer(const std::chrono::milliseconds& ms, const std::function<bool ()>& timerFunc))",
        [](const std::string& browser) {
        CONSTRUCT_UI
        ui.startTimer(1000ms, true, [&ui]()  {
           ui.exit();
        });
        ui.run();
        return true;
}},
{R"(bool stopTimer(TimerId))",
        [](const std::string& browser) {
    CONSTRUCT_UI
    bool ok = true;
    auto id = ui.startTimer(1000ms, true, [&ui, &ok]()  {
       ok = false;
       ui.exit();
    });
    ui.startTimer(3000ms, true, [&ui]() {
          ui.exit();
       });
    ui.stopTimer(id);
    ui.run();
    return ok;
}},
{R"(Element root() const)",
        [](const std::string& browser) {
    CONSTRUCT_UI
    return ui.root().id() == ""; //root has no id
}},
{R"(std::string addressOf(const std::string& filepath) const)",
        [](const std::string& browser){
    const std::string html = TelexUtils::systemEnv("TELEX-UNITTEST-HTML") ;
    if(html.empty()) {
        std::cerr << "no TELEX-UNITTEST-HTML set" << std::endl;
        return false;
    }
    CONSTRUCT_UI
    return ui.addressOf(html).length() > 0;
}},
{R"(std::optional<Element::Elements> byClass(const std::string& className) const)",
[](const std::string& browser) {
    CONSTRUCT_UI
    bool ok = false;
    ui.onOpen([&ok, &ui](
              ) {
        const auto classes = ui.byClass("test-class");
        ok = classes.has_value() && classes->size() == 4; //4 test-classes
        ui.exit();
    });
    ui.run();
    return ok;
}},
{R"(std::optional<Element::Elements> byName(const std::string& className) const)",
[](const std::string& browser) {
    CONSTRUCT_UI
    bool ok = false;
    ui.onOpen([&ok, &ui](){
        const auto names = ui.byName("test-name");
        ok = names.has_value() && names->size() == 5; //5 test-classes
        ui.exit();
    });
    ui.run();
    return ok;
}},
{R"(std::optional<std::pair<std::chrono::milliseconds, std::chrono::milliseconds>> ping() const)",
        [](const std::string& browser) {
    CONSTRUCT_UI
    bool ok = false;
    ui.onOpen([&ok, &ui](){
        const auto ping = ui.ping();
        ok = ping.has_value() && ping->first.count() > 0 && ping->second.count() > 0 && ping->first.count() < 10000 && ping->second.count() < 10000;
        ui.exit();
    });
    ui.run();
    return ok;
}},
{R"(std::vector<uint8_t> resource(const std::string& url))",
        [](const std::string& browser){
    CONSTRUCT_UI
    const auto r = ui.resource("/test.html");
    if(!r)
        return false;
    const std::string html = TelexUtils::join(*r);
    const auto p1 = html.find("html");
    const auto p2 = html.find("html");
    return p1 == p2;
}},
{R"(bool addFile(const std::string& url, const std::string& file)",
        [](const std::string& browser){
    CONSTRUCT_UI
    const std::string test = "The quick brown fox jumps over the lazy dog";
    const auto tempFile = TelexUtils::writeToTemp(test);
    const auto ok = ui.addFile("test_data", tempFile);
    if(!ok)
        return false;
    TelexUtils::removeFile(tempFile);
    const auto r = ui.resource("test_data");
    const std::string text = TelexUtils::join(*r);
    const auto p1 = text.find("quick");
    return p1 != std::string::npos && text.length() == test.length();
}}

        };

const std::vector<std::tuple<std::string, std::function<bool (const std::string& browser)>>>Eltests = {
{R"(Element(Ui& ui, const std::string& id))",
        [](const std::string& browser) {
    CONSTRUCT_UI
    Telex::Element foo(ui, "test-1");
    return true;
}},
{R"(Element(Ui& ui, const std::string& id, const std::string& htmlElement, Element& parent))",
        [](const std::string& browser) {
        CONSTRUCT_UI
        bool ok = false;
        ui.onOpen([&ok, &ui]() {
            Telex::Element parent(ui, "test-1");
            Telex::Element(ui, "boing", "div", parent);
            const auto cop = parent.children();
            ok = cop.has_value() && std::find_if(cop->begin(), cop->end(), [](const auto el ){return el.id() == "boing";}) != cop->end();
            ui.exit();
        });
        ui.run();
        return ok;}},
{R"(std::string id() const)",
        [](const std::string& browser){
    CONSTRUCT_UI
    Telex::Element foo(ui, "test-1");
    return foo.id() == "test-1";
}},
{R"(Element& subscribe(const std::string& name, Handler handler))",
        [](const std::string& browser) {
        CONSTRUCT_UI
        bool ok = false;
        Telex::Element el(ui, "test-1");
        el.subscribe("test_event", [&ok, &ui, &el](const Telex::Element::Event& eel) {
            ok = eel.element->id() == el.id();
            ui.exit();
        });
        ui.startTimer(2s, true, [&el]()  {
              el.setAttribute("style", "color:green");
           });
        ui.startTimer(3s, true, [&ui]()  {
              ui.exit();
           });
        ui.run();
        return ok;
}},
{R"(Element& setHTML(const std::string& htmlText))",
        [](const std::string& browser) {
    CONSTRUCT_UI
    Telex::Element el(ui, "test-1");
    el.setHTML("Test-dyn");
    bool ok = false;
    ui.onOpen([&ok, &ui, &el]() {
        const auto html = el.html();
        ok = html.has_value() && html.value() == "Test-dyn";
        ui.exit();
    });
    ui.run();
    return ok;
}},
{R"(Element& setAttribute(const std::string& attr, const std::string& values))",
        [](const std::string& browser) {
    CONSTRUCT_UI
    Telex::Element el(ui, "test-1");
    el.setAttribute("value", "Test-attr-dyn");
    bool ok = false;
    ui.onOpen([&ok, &ui, &el]() {
        const auto attrs = el.attributes();
        ok = attrs.has_value()
                && attrs->find("value") != attrs->end()
                && attrs.value().find("value")->second == "Test-attr-dyn";
        ui.exit();
    });
    ui.run();
    return ok;
}},
{R"(std::optional<Attributes> attributes() const)",
        [](const std::string& browser) {
    CONSTRUCT_UI
    bool ok = false;
    ui.onOpen([&ok, &ui]() {
        Telex::Element el(ui, "test-1");
        const auto attrs = el.attributes();
        ok = attrs.has_value()
                && attrs->find("value") != attrs->end()
                && attrs->find("value")->second == "Test-attr";
        ui.exit();
    });
    ui.run();
    return ok;
}},
{R"(std::optional<Elements> children() const)",
        [](const std::string& browser) {
    CONSTRUCT_UI
    bool ok = false;
    ui.onOpen([&ok, &ui]() {
        Telex::Element el(ui, "test-1");
        const auto chlds = el.children();
        ok = chlds.has_value()
                && chlds->size() > 2
                && (*chlds)[2].id() == "test-child-2";
        ui.exit();
    });
    ui.run();
    return ok;
}},
{R"(std::optional<Values> values() const)",
        [](const std::string& browser) {
    CONSTRUCT_UI
    bool ok = false;
    ui.onOpen([&ok, &ui]() {
        Telex::Element el(ui, "checkbox-1");
        const auto values = el.values();
        ok = values.has_value()
                && values->find("checked") != values->end()
                && values->find("checked")->second == "true";
        ui.exit();
    });
    ui.run();
    return ok;
}},
{R"(std::optional<std::string> html() const)",
        [](const std::string& browser) {
    CONSTRUCT_UI
    bool ok = false;
    ui.onOpen([&ok, &ui]() {
        Telex::Element el(ui, "test-1");
        const auto html = el.html();
        ok = html.has_value() && html.value().find("Test-1") != std::string::npos;
        ui.exit();
    });
    ui.run();
    return ok;
}},
{R"(void remove())",
        [](const std::string& browser) {
    CONSTRUCT_UI
    bool ok = false;
    ui.onOpen([&ok, &ui]() {
        Telex::Element el(ui, "test-1");
        const auto chlds = el.children();
        ok = chlds.has_value()
                && chlds->size() > 3
                && (*chlds)[3].id() == "test-child-3";
        if(ok) {
            Telex::Element c(ui,"test-child-3");
            c.remove();
            const auto cop = el.children();
            ok = cop.has_value() && std::find_if(cop->begin(), cop->end(), [](const auto el ){return el.id() == "test-child-3";}) == cop->end();
        }
        ui.exit();
    });
    ui.run();
    return ok;
}}};


static std::unordered_map<unsigned, std::string> nonRun {
    {1, "needs an env"},
    {2, "needs an env"},
    {3, "Needs an env"},
    {8, "Broken test"},
    {20, "Needs an env"},
    {14, "Needs user interaction"},
    {27, "Broken test"},
    {28, "Broken test"}
};

int main(int args, char* argv[]) {
    const auto tests =  TelexUtils::merge(Uitests, Eltests);
    const auto alist = TelexUtils::parseArgs(args, argv, {
                                            {"tests", 't', TelexUtils::ArgType::REQ_ARG},
                                            {"skipped", 's', TelexUtils::ArgType::REQ_ARG},
                                            {"debug", 'd', TelexUtils::ArgType::NO_ARG},
                                            {"chrome", 'c', TelexUtils::ArgType::REQ_ARG},
                                            {"headless", 'h', TelexUtils::ArgType::NO_ARG}
                                        });
    const auto params = std::get_if<TelexUtils::Params>(&alist);
    std::set<unsigned> skipped;
    std::string browser;
    if(params) {
        const auto opts = std::get<TelexUtils::Options>(*params);
        if(TelexUtils::contains(opts, "debug")) {
            Telex::setDebug();
        }
        if(TelexUtils::contains(opts, "browser")) {
            browser = TelexUtils::qq(opts.find("browser")->second)  + " " + headlessParams();
        }
        if(TelexUtils::contains(opts, "headless")) {
            browser = defaultChrome() + " " + headlessParams();
            nonRun.emplace(5, "Not work on headless");
            nonRun.emplace(6, "Not work on headless");
            nonRun.emplace(9, "Not work on headless");
            nonRun.emplace(10, "Not work on headless");
            nonRun.emplace(12, "Not work on headless");
            nonRun.emplace(21, "Not work on headless");
            nonRun.emplace(22, "Not work on headless");
            nonRun.emplace(23, "Not work on headless");
            nonRun.emplace(29, "Not work on headless");
            nonRun.emplace(30, "Not work on headless");
            nonRun.emplace(31, "Not work on headless");
            nonRun.emplace(32, "Not work on headless");
            nonRun.emplace(33, "Not work on headless");
            nonRun.emplace(35, "Not work on headless");
            nonRun.emplace(34, "Not work on headless");
            nonRun.emplace(36, "Not work on headless");
        }
        const auto lp = opts.find("tests");
        if(lp != opts.end()) {
            for(auto i = 1U; i <= tests.size(); i++)
                skipped.emplace(i);
            const auto list = TelexUtils::split<std::vector<std::string>>(lp->second, ',');
            for(const auto& l : list) {
                const auto lop = TelexUtils::toOr<unsigned>(l);
                if(lop.has_value()) {
                    const auto index = lop.value();
                    const auto it = skipped.find(index);
                    skipped.erase(it);
                }
            }
        }
        const auto sp = opts.find("skipped");
        if(sp != opts.end()) {
            const auto list = TelexUtils::split<std::vector<std::string>>(sp->second, ',');
            for(const auto& l : list) {
                const auto lop = TelexUtils::toOr<unsigned>(l);
                if(lop.has_value()) {
                    const auto index = lop.value();
                    skipped.emplace(index);
                }
            }
        }
    }

    auto testNo = 0U;
    int skippedTotal = 0;
    int failedTotal = 0;
    int successTotal = 0;
    for(const auto& [n, f] : tests) {
        ++testNo;
        if(skipped.find(testNo) != skipped.end()) {
            std::cout << "skip " << testNo << std::endl;
            ++skippedTotal;
            continue;
        }
        if(nonRun.find(testNo) != nonRun.end()) {
            std::cout << "not run " << testNo << " due " << nonRun.find(testNo)->second << std::endl;
            ++skippedTotal;
            continue;
        }
        std::cout << testNo << " ";
        if(f) {
            const auto result = std::time(nullptr);
            std::cout << "Execute test for " << n << "..." << '[' << TelexUtils::chop(std::asctime(std::localtime(&result))) << "]" << std::flush;
            std::cerr << std::flush;
            auto wait = TelexUtils::waitExpire(30s, [testNo, &failedTotal](){
                 const auto result = std::time(nullptr);
                 std::cerr << "Test " << testNo << " took too long " << "fail " << '[' << TelexUtils::chop(std::asctime(std::localtime(&result))) << "]" << std::endl;
                 ++failedTotal;
            });
            const bool success = f(browser);
            if(success) {
                ++successTotal;
                std::cout << "ok" << "\n" << std::endl;
            } else {
                ++failedTotal;
                std::cout <<  "fail" << "\n" << std::endl;
            }

        } else {
             std::cerr << "Test for " << n << " not implemented" << std::endl;
              ++skippedTotal;
        }
    }
    std::cout << "skipped:" << skippedTotal << " failed:" << failedTotal << " passed:" << successTotal << std::endl;
    return 0;
}

