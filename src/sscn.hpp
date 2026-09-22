#ifndef AMITG_FC_SERVICE_STATUS_CHANGED_NOTIFIER
#define AMITG_FC_SERVICE_STATUS_CHANGED_NOTIFIER

/*
    sscn.hpp
    Copyright (c) 2024-2026, Amit Gefen

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal in the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

// clang-format off
// NOLINTBEGIN(misc-include-cleaner)
#include <Windows.h>
// clang-format on

#include <functional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace amitgdev {

class ServiceStatusChangedNotifier final {
 public:
  using ActionFunction = std::function<void(const std::wstring& service_name,
                                            DWORD current_state)>;

  ServiceStatusChangedNotifier() = default;

  ~ServiceStatusChangedNotifier() { Stop(); }  // (Non-default destructor)

  // (rule of 5) Since non-default destructor:
  // Delete copy/move constructor and copy assignment operator
  ServiceStatusChangedNotifier(const ServiceStatusChangedNotifier&) = delete;
  ServiceStatusChangedNotifier&
  operator=(const ServiceStatusChangedNotifier&) = delete;
  ServiceStatusChangedNotifier(ServiceStatusChangedNotifier&&) = delete;
  ServiceStatusChangedNotifier&
  operator=(ServiceStatusChangedNotifier&&) = delete;

  // Subscribe to SC_EVENT_STATUS_CHANGE notifications for the specified
  // services.
  //
  // `action_function` is invoked from NotifyCallbackFunc(), a static
  // callback running on an arbitrary thread with no owning caller frame to
  // unwind into - so it must not throw. That's enforced here, at the call
  // site, before the callable is type-erased into ActionFunction (an
  // already-erased std::function's call operator can't be checked this
  // way, since erasure discards the noexcept-ness of the wrapped target).
  template <typename F>
    requires std::is_nothrow_invocable_v<F, const std::wstring&, DWORD>
  void Start(const std::vector<std::wstring>& service_list, DWORD notify_mask,
             F&& action_function) {
    StartImpl(service_list, notify_mask,
              ActionFunction(std::forward<F>(action_function)));
  }

  // Unsubscribe from all service notifications.
  void Stop() noexcept;

 private:
  // Does the actual subscription work. Not noexcept: populating
  // service_data_map_ can throw (e.g. bad_alloc) on allocation failure, and
  // that's the caller's exception to handle - this runs on the caller's own
  // thread, unlike NotifyCallbackFunc().
  void StartImpl(const std::vector<std::wstring>& service_list,
                 DWORD notify_mask, ActionFunction action_function);

  // Tailored context for NotifyCallbackFunc():
  using Context = struct {
    DWORD notify_mask;
    ActionFunction action_function;
  };

  // Keeps data (per monitored service) *that has to be persistent* as long as
  // monitoring intact.
  struct ServiceData {
    wchar_t service_name[MAX_PATH + 1]{
        L'\0',
    };  // ('non-const' wchar-string to be pointed by:
        // notify_buffer->pszServiceNames)
    SERVICE_NOTIFY
    notify_buffer{};  // That does NOT initialize all elements of the struct
                      // to zero! Rather, it initializes the struct to its
                      // default values.
    PSC_NOTIFICATION_REGISTRATION registration{nullptr};
    DWORD system_error_code{ERROR_SUCCESS};
  };

  std::unordered_map<std::wstring, ServiceData>
      service_data_map_;  // Key: service_name, Value: SERVICE_DATA (see
                          // above).

  Context context_{};

  static VOID CALLBACK NotifyCallbackFunc(_In_ DWORD notify,
                                          _In_ PVOID callback_context) noexcept;

  // SecHost.dll function wrappers:

  // SubscribeServiceChangeNotifications_wrapper()
  // Has the signature of SecHost.dll SubscribeServiceChangeNotifications()
  // Calls SubscribeServiceChangeNotifications()
  // Returns: SubscribeServiceChangeNotifications() return value
  // If failed before, returns -1.
  static DWORD WINAPI SubscribeServiceChangeNotificationsWrapper(
      _In_ SC_HANDLE service, _In_ SC_EVENT_TYPE event_type,
      _In_ PSC_NOTIFICATION_CALLBACK callback, _In_opt_ PVOID callback_context,
      _Out_ PSC_NOTIFICATION_REGISTRATION* subscription);

  // UnsubscribeServiceChangeNotifications_wrapper()
  // Has the signature of SecHost.dll UnsubscribeServiceChangeNotifications()
  // Calls UnsubscribeServiceChangeNotifications()
  static VOID WINAPI UnsubscribeServiceChangeNotificationsWrapper(
      _In_ PSC_NOTIFICATION_REGISTRATION subscription);
};

// NOLINTEND(misc-include-cleaner)

}  // namespace amitgdev

#endif