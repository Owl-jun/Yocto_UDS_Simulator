#include "DiagnosticServer.hpp"

#include "Logger.hpp"

DiagnosticServer::DiagnosticServer(DiagnosticConfig config)
    : config_(std::move(config))
    , dispatcher_(did_manager_, session_manager_)
    , tcp_server_(config_.port, [this](const std::string& request) {
        return dispatcher_.handle_line(request);
    })
{
    did_manager_.set(0xF190, config_.vin);
    did_manager_.set(0xF187, "SW-1.0.0");
    did_manager_.set(0xF188, "HW-QEMU-X86_64");
    did_manager_.set(0xF18C, "SIM-0001");
}

void DiagnosticServer::run()
{
    Logger::info("Diagnostic daemon starting");
    tcp_server_.run();
}
