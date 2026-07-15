#include "DiagnosticServer.hpp"

#include "Logger.hpp"

DiagnosticServer::DiagnosticServer(DiagnosticConfig config)
    : config_(std::move(config))
    , dispatcher_(did_manager_, dtc_manager_, session_manager_)
    , tcp_server_(config_.port, [this](const std::string& request) {
        const std::lock_guard<std::mutex> lock(dispatcher_mutex_);
        return dispatcher_.handle_line(request);
    })
{
    did_manager_.reset_defaults(config_.vin);
    dtc_manager_.reset_defaults();
}

void DiagnosticServer::run()
{
    Logger::info("Diagnostic daemon starting");
    tcp_server_.run();
}
