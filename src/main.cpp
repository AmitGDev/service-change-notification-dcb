// *RUN "AS ADMIN"!*

#include "sscn.hpp"

// clang-format off
// NOLINTBEGIN(misc-include-cleaner)
#include <Windows.h>
// clang-format on

#include <cstdlib>
#include <exception>
#include <iostream>
#include <syncstream>
#include <thread>

// Implement what to do on service status-changed notification.
//
// Constructing/streaming through std::wosyncstream can throw (e.g. a locale
// facet failure). This callable runs, via Start()'s nothrow-invocable
// requirement, from the same OS-callback boundary as NotifyCallbackFunc, with
// no owning caller frame to unwind into - the try/catch below is what
// actually stops that from escaping; the check just can't see it.
// NOLINTNEXTLINE(bugprone-exception-escape)
static void OnNotificationActionFunction(const std::wstring& service_name,
                                         const DWORD& current_state) noexcept {
  try {
    std::wosyncstream sync_stream(std::wcout);  // (Since C++20)

    sync_stream << L"notification: " << service_name << L" current state: "
                << current_state << '\n';

    if (current_state ==
        SERVICE_NOTIFY_STOPPED) {  // Only if service is stopped:
      sync_stream << L"action" << '\n';
    }
  } catch (...) {
    // Drop this one notification rather than let anything escape this
    // boundary. Logged via a call that cannot itself throw (unlike
    // std::wosyncstream above).
    OutputDebugStringW(
        L"OnNotificationActionFunction: dropped a notification due to an "
        L"exception.\n");
  }
}

// *RUN "AS ADMIN"!*
//
// Constructing ServiceStatusChangedNotifier (its unordered_map) can throw on
// allocation failure. The try/catch below is what actually turns that into a
// clean, reported exit instead of an implicit terminate; the check just
// can't see it.
// NOLINTNEXTLINE(bugprone-exception-escape)
int main() {
  try {
    amitgdev::ServiceStatusChangedNotifier service_status_change_notifier;

    // Start (and subscribe to "W32Time" and "WebClient"):
    service_status_change_notifier.Start(
        std::vector<std::wstring>{
            L"W32Time",
            L"WebClient",
        },                       // A vector of service names.
        SERVICE_NOTIFY_STOPPED,  // Notify about service STOPPED (Notify Mask).
        OnNotificationActionFunction);  // <-- Notify to this function (see
                                        // above).

    // Provide 5 minutes to manually Start / Stop "W32Time" and "WebClient"
    // services and to check the functionality.
    std::this_thread::sleep_for(std::chrono::minutes(5));

    // Exit (unsubscribe all):
    service_status_change_notifier
        .Stop();  // Test: Set BP on Sleep(). On break, Set-Next-Statement here
                  // + single-step (to see that the WT exited).
  } catch (const std::exception& ex) {
    std::cerr << "Unhandled exception: " << ex.what() << '\n';
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Unhandled exception of unknown type.\n";
    return EXIT_FAILURE;
  }
}

// NOLINTEND(misc-include-cleaner)
