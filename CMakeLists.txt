cmake_minimum_required(VERSION 3.16)

project(diagnostic_ecu LANGUAGES CXX)

include(GNUInstallDirs)
find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SYSTEMD_SYSTEM_UNIT_DIR
    "${CMAKE_INSTALL_LIBDIR}/systemd/system"
    CACHE PATH "systemd system unit install directory"
)

add_library(diagnostic_core STATIC
    app/Config.cpp
    app/DiagnosticServer.cpp
    app/DidManager.cpp
    app/DtcManager.cpp
    app/Logger.cpp
    app/SessionManager.cpp
    app/TcpServer.cpp
    app/UdsDispatcher.cpp
)

target_include_directories(diagnostic_core PUBLIC include)
target_link_libraries(diagnostic_core PUBLIC Threads::Threads)

target_compile_options(diagnostic_core PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_executable(diagnosticd app/main.cpp)
target_link_libraries(diagnosticd PRIVATE diagnostic_core)

install(TARGETS diagnosticd RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES config/diagnostic.conf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR})
install(FILES systemd/diagnostic.service DESTINATION ${SYSTEMD_SYSTEM_UNIT_DIR})

include(CTest)

if(BUILD_TESTING)
    add_executable(uds_dispatcher_test tests/UdsDispatcherTest.cpp)
    target_link_libraries(uds_dispatcher_test PRIVATE diagnostic_core)
    add_test(NAME uds_dispatcher_test COMMAND uds_dispatcher_test)
endif()
