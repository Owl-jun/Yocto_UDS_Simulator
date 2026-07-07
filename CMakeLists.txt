cmake_minimum_required(VERSION 3.16)

project(diagnostic_ecu LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SYSTEMD_SYSTEM_UNIT_DIR
    "${CMAKE_INSTALL_LIBDIR}/systemd/system"
    CACHE PATH "systemd system unit install directory"
)

add_executable(diagnosticd
    app/main.cpp
    app/Config.cpp
    app/DiagnosticServer.cpp
    app/DidManager.cpp
    app/Logger.cpp
    app/SessionManager.cpp
    app/TcpServer.cpp
    app/UdsDispatcher.cpp
)

target_include_directories(diagnosticd PRIVATE include)

target_compile_options(diagnosticd PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

install(TARGETS diagnosticd RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES config/diagnostic.conf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR})
install(FILES systemd/diagnostic.service DESTINATION ${SYSTEMD_SYSTEM_UNIT_DIR})
