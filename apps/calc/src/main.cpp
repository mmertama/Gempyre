#include <telex.h>
#include <telex_utils.h>
#include <cstdlib>
#include <tuple>
#include <cmath>
#include "computor.h"
#include "calc_resource.h"

std::string trim(const std::string& s) {
    auto f = s.find_first_not_of('0');
    return f == std::string::npos ? "0" : s.substr(f);
}

Telex::Element::Elements getClass(Telex::Ui& ui, const std::string& name) {
    auto els = ui.byClass(name);
    telex_utils_assert_x(els.has_value(), "Cannot get " + name);
    auto v = els.value();
    telex_utils_assert_x(!v.empty(), "Cannot find " + name);
    return v;
}

int main(int /*argc*/, char** /*argv*/) {
 //   Telex::setDebug();
    Computor::Computor comp;
 /*   std::vector<std::string> input = {"0", "0", "1", "2", "0", ".", ".", "3", "4", "+", "2", "1", "=", Computor::Sqrt, "3", "9", Computor::Exp2, ".", Computor::Denom, "9", Computor::Denom, "6", Computor::Exp, "="};
    Computor::Computor comp;
    for(const auto& i : input) {
        const auto r =  comp.push(i);
        Utils::log(Utils::LogLevel::Info, "Add", i, "out is", r.has_value() ? toString(r.value()) : "ERR");
    }
     return 1;
    */

    const std::string miniview = TelexUtils::systemEnv("TELEX-EXTENSION") ;
	if(miniview.empty()) {
        TelexUtils::log(TelexUtils::LogLevel::Error, "TELEX-EXTENSION is not set");
	}
    else if(!TelexUtils::fileExists(miniview)) {
        TelexUtils::log(TelexUtils::LogLevel::Error, "TELEX-EXTENSION", miniview, "is not found");
	}
    Telex::Ui ui({{"/calc.html", Calchtml}, {"/calc.css", Calccss}, {"/calc.png", Calcpng}},
                 "calc.html",
                 miniview,
                 miniview.empty() ? "" : "280 355 Calculator");


    auto operation = [&comp](Telex::Element& screenEl, Telex::Element& resultEl, const std::string& op) mutable {
        const auto r = comp.push(op);
        screenEl.setHTML(r.has_value() ? Computor::toString(r.value()) : "ERR");
        const auto m = comp.memory();
        resultEl.setHTML(m.has_value() ? Computor::toString(m.value()) : "");
    };

    ui.onOpen([&ui, operation]() mutable {
        auto total = getClass(ui, "total")[0];
        total.setHTML("0");
        auto last = getClass(ui, "last")[0];
        const auto ce = getClass(ui, "calculator")[0];
        std::function<void (const Telex::Element& )> getChildren;
        getChildren = [total, last, operation, &getChildren] (const Telex::Element& ce) mutable {
            auto children = ce.children();
            telex_utils_assert_x(children.has_value(), "Cannot find calculator elements");
            for(auto& el : children.value()) {
                TelexUtils::log(TelexUtils::LogLevel::Info, "el id", el.id());
                const auto atv = el.attributes();
                telex_utils_assert_x(atv.has_value(), "Cannot read calculator element attributes");
                const auto attrs = atv.value();
                TelexUtils::log(TelexUtils::LogLevel::Info, "attributes", el.id(), TelexUtils::joinPairs(attrs.begin(), attrs.end()));
                auto vit = attrs.find("class");
                if(vit != attrs.end()) {
                    const auto pos = vit->second.find("calc_");
                    if(pos != std::string::npos) {
                         const auto value = attrs.find("value");
                         telex_utils_assert_x(value != attrs.end(), "button has no value");
                         const auto key = value->second;
                         el.subscribe("click", [key, total, last, operation] (const Telex::Event&) mutable {
                            operation(total, last, key);
                         });
                     }
                 }
                getChildren(el);
            }
        };
        getChildren(ce);
    });
    ui.run();
    return 0;
}
