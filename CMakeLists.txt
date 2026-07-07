cmake_minimum_required(VERSION 3.16)

project(diagnostic_ecu LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

install(TARGETS diagnosticd RUNTIME DESTINATION bin)
install(FILES config/diagnostic.conf DESTINATION etc)
install(FILES systemd/diagnostic.service DESTINATION lib/systemd/system)
