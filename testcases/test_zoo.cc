#include "rocket/common/zookeeper_util.h"
#include "rocket/common/log.h"
#include <cstring>
#include <iostream>

int main() {
    rocket::Config::SetGlobalConfig("../conf/rocket_client.xml");
    rocket::Logger::InitGlobalLogger(0);

    rocket::ZkClient zkCli;
    zkCli.Start();

    std::string method_path = "/Order_Stub/makeOrder";

    std::string host_data = zkCli.GetData(method_path.c_str());

    std::cout << host_data << std::endl;
}