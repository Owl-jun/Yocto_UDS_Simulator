#pragma once

#include "Config.hpp"
#include "DidManager.hpp"
#include "DtcManager.hpp"
#include "SessionManager.hpp"
#include "TcpServer.hpp"
#include "UdsDispatcher.hpp"

#include <mutex>

class DiagnosticServer {
public:
    explicit DiagnosticServer(DiagnosticConfig config);

    void run();

private:
    DiagnosticConfig config_;
    DidManager did_manager_;
    DtcManager dtc_manager_;
    SessionManager session_manager_;
    UdsDispatcher dispatcher_;
    std::mutex dispatcher_mutex_;
    TcpServer tcp_server_;
};
