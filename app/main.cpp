#include "Config.hpp"
#include "DiagnosticServer.hpp"
#include "Logger.hpp"

#include <exception>
#include <string>

int main(int argc, char* argv[])
{
    const std::string config_path = argc > 1 ? argv[1] : "config/diagnostic.conf";

    try {
        const auto config = Config::load(config_path);
        DiagnosticServer server(config);
        server.run();
    } catch (const std::exception& error) {
        Logger::error(error.what());
        return 1;
    }

    return 0;
}
