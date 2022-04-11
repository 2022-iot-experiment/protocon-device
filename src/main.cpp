#include <Protocon/Protocon.h>
#include <Protocon/Request.h>
#include <Protocon/Response.h>
#include <Protocon/SignInResponse.h>
#include <spdlog/spdlog.h>

#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <ios>
#include <sstream>
#include <string>
#include <thread>

std::string getline(std::ifstream& stream) {
    static std::array<char, 105> buffer;
    if (!stream.getline(buffer.data(), buffer.size()).good()) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        throw 0;
    }

    buffer[std::strlen(buffer.data()) - 1] = '\0';

    return '[' + std::string(buffer.data()) + ']';
}

std::string getData(std::ifstream& stream) {
    std::string res("[");
    const int n = 1;
    for (int i = 0; i < n; i++) {
        std::string line = getline(stream);
        std::string sensor_id = line.substr(1, 4);

        if (sensor_id != std::string("6636"))
            i--;
        else {
            if (i != 0) res += ',';
            res += line;
        }
    }
    return res + ']';
}

int main() {
    bool trigger = false;

    auto gw = Protocon::GatewayBuilder(2)
                  .withSignInResponseHandler([&trigger](const Protocon::SignInResponse& r) {
                      if (!r.status)
                          trigger = true;
                  })
                  .build();

    auto tk = gw.createClientToken();

    if (!gw.run("127.0.0.1", 8082)) return 1;

    spdlog::info("连接服务器成功");

    std::ifstream stream("/home/hebo/Projects/protocon-device/sensor_sample_float_output.csv", std::ios::in);

    while (gw.isOpen()) {
        gw.poll();

        if (trigger) {
            gw.send(tk, Protocon::Request{
                            static_cast<uint64_t>(time(nullptr)),
                            0x0004,
                            std::string("{\"data\": " + getData(stream) + std::string("}")),
                        },
                    [](const Protocon::Response& r) {});
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return 0;
}
