#pragma once

#include "Config.hpp"
#include "DidManager.hpp"
#include "SessionManager.hpp"
#include "TcpServer.hpp"
#include "UdsDispatcher.hpp"

class DiagnosticServer {
public:
    explicit DiagnosticServer(DiagnosticConfig config);

    void run();

private:
    DiagnosticConfig config_;
    DidManager did_manager_;
    SessionManager session_manager_;
    UdsDispatcher dispatcher_;
    TcpServer tcp_server_;
};
