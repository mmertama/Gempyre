#include "gempyre.h"
#include "gempyre_utils.h"
#include <ifaddrs.h>
#include <vector>

#include "affiliates_test_rpc_resource.h"

int main(int argc, char* argv[]) {
    struct ifaddrs *id;
    ::getifaddrs(&id);
    const auto hostName = std::string(argc > 1 ? argv[1] : "127.0.0.1");
    const auto commandLine = GempyreUtils::join(std::vector<std::string>({std::string("host=") + hostName,
    "port=43101", std::string(id->ifa_addr), "400", "400", "Remote, but near"}), " ");

    const std::string python3 = GempyreUtils::which("python3").isEmpty() ? "python" : "python3";
    const auto rpc = argv[0] + "/uisrvclient.py";
    gempyre_utils_fatal_f(rpc + " not found", [&rpc](){GempyreUtils::fileExists(rpc);});
    const auto ui = Gempyre::Ui({{"/main.html", Affiliates_test_rpchtml}},
     "main.html",
     GempyreUtils::join({python3, rpc}, " ")
     "500 640 \"Test RPC\"");
    const auto host_name = Gempyre::Element("host", ui)
    host_name.setHTML(id->ifa_name);
    ::freeifaddrs(id);
    ui.run()
}


