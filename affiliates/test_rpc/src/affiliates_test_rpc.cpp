#include "gempyre.h"
#include "gempyre_utils.h"
#include <vector>
#include <iostream>

#include "affiliates_test_rpc_resource.h"

int main(int argc, char* argv[]) {
    const auto hostName = std::string(argc > 1 ? argv[1] : "127.0.0.1");
    const auto localAddr = GempyreUtils::ipAddresses(GempyreUtils::AddressType::Ipv4);
    const auto commandLine = GempyreUtils::join(std::vector<std::string>({std::string("host=") + hostName,
    "port=43101", std::string(localAddr[0]), "400", "400", "Remote, but near"}), " ");

    const std::string python3 = GempyreUtils::which("python3").empty() ? "python" : "python3";


    const auto rpc = GempyreUtils::pathPop(argv[0], 2) + std::string("/py_uisrv/ui_srvclient.py");
    gempyre_utils_assert_x(GempyreUtils::fileExists(rpc), rpc + " not found");

    auto ui = Gempyre::Ui({{"/main.html", Affiliates_test_rpchtml}},
     "main.html", python3 + " " + rpc, commandLine);

    auto nameElement = Gempyre::Element(ui, "host");
    nameElement.setHTML(GempyreUtils::join(localAddr, "</br>"));
    ui.run();
}


