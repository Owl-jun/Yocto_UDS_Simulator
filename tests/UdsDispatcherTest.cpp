#include "DidManager.hpp"
#include "DtcManager.hpp"
#include "SessionManager.hpp"
#include "UdsDispatcher.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

class DispatcherFixture {
public:
    DispatcherFixture()
        : dispatcher(did_manager, dtc_manager, session_manager)
    {
        did_manager.reset_defaults("KMH00000000000000");
        dtc_manager.reset_defaults();
    }

    DidManager did_manager;
    DtcManager dtc_manager;
    SessionManager session_manager;
    UdsDispatcher dispatcher;
};

void expect_eq(const std::string& actual, const std::string& expected, const std::string& label)
{
    if (actual == expected) {
        return;
    }

    std::cerr << label << "\nexpected: " << expected << "\nactual:   " << actual << '\n';
    std::exit(EXIT_FAILURE);
}

void test_clear_dtc_requires_extended_session()
{
    DispatcherFixture fixture;

    expect_eq(fixture.dispatcher.handle_line("14 FF FF FF"), "7F 14 22", "clear DTC before extended session");
    expect_eq(fixture.dispatcher.handle_line("10 03"), "50 03", "enter extended session");
    expect_eq(fixture.dispatcher.handle_line("14 FF FF FF"), "54", "clear all DTCs");
    expect_eq(fixture.dispatcher.handle_line("19 02 FF"), "59 02 FF", "DTC list after clear");
}

void test_did_write_requires_extended_session()
{
    DispatcherFixture fixture;

    expect_eq(fixture.dispatcher.handle_line("2E F1 8C 41 42 43"), "7F 2E 22", "write DID before extended session");
    expect_eq(fixture.dispatcher.handle_line("10 03"), "50 03", "enter extended session for DID write");
    expect_eq(fixture.dispatcher.handle_line("2E F1 8C 41 42 43"), "6E F1 8C", "write serial number DID");
    expect_eq(fixture.dispatcher.handle_line("22 F1 8C"), "62 F1 8C 41 42 43", "read serial number DID");
}

void test_security_access_and_routine_control()
{
    DispatcherFixture fixture;

    expect_eq(fixture.dispatcher.handle_line("31 01 FF 00"), "7F 31 22", "routine before security access");
    expect_eq(fixture.dispatcher.handle_line("10 03"), "50 03", "enter extended session for routine");
    expect_eq(fixture.dispatcher.handle_line("27 02 ED CB"), "7F 27 24", "send key before seed");
    expect_eq(fixture.dispatcher.handle_line("27 01"), "67 01 12 34", "request seed");
    expect_eq(fixture.dispatcher.handle_line("27 02 00 00"), "7F 27 35", "invalid key");
    expect_eq(fixture.dispatcher.handle_line("27 01"), "67 01 12 34", "request seed again");
    expect_eq(fixture.dispatcher.handle_line("27 02 ED CB"), "67 02", "send valid key");
    expect_eq(fixture.dispatcher.handle_line("31 01 FF 00"), "71 01 FF 00 00", "start routine");
    expect_eq(fixture.dispatcher.handle_line("31 03 FF 00"), "71 03 FF 00 01", "routine running");
    expect_eq(fixture.dispatcher.handle_line("31 02 FF 00"), "71 02 FF 00 00", "stop routine");
    expect_eq(fixture.dispatcher.handle_line("31 03 FF 00"), "71 03 FF 00 00", "routine stopped");
}

void test_invalid_request_handling()
{
    DispatcherFixture fixture;

    expect_eq(fixture.dispatcher.handle_line("99"), "7F 99 11", "unsupported service");
    expect_eq(fixture.dispatcher.handle_line("10"), "7F 10 13", "short session request");
    expect_eq(fixture.dispatcher.handle_line("0x10 0x03"), "50 03", "0x-prefixed tokens");
}

} // namespace

int main()
{
    test_clear_dtc_requires_extended_session();
    test_did_write_requires_extended_session();
    test_security_access_and_routine_control();
    test_invalid_request_handling();

    return EXIT_SUCCESS;
}
