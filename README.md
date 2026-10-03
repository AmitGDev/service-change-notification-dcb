# Windows Service Notifications - Direct Callback (DCB)

A C++23 Windows component for monitoring the state of selected Windows services asynchronously through the Service Control Manager (SCM).

The project provides `ServiceStatusChangedNotifier`, a small notifier that registers service-change notifications for selected services and invokes an application-defined action when a requested service-state transition occurs.

## How It Works

The notifier:

1. Opens the Windows Service Control Manager.
2. Opens the services selected by the caller.
3. Registers `SC_EVENT_STATUS_CHANGE` notifications for the requested notification mask.
4. Receives notifications through the registered SCM callback.
5. Dispatches the resulting service-state change to the caller's action function.

The notification callback is registered directly with the Service Control Manager:

```cpp
SubscribeServiceChangeNotifications(
    service,
    SC_EVENT_STATUS_CHANGE,
    NotifyCallbackFunc,
    notify_buffer,
    &registration);
```

The callback receives the service name and notification value and invokes the application-defined action when the requested notification condition is met.

## Example

The example monitors two Windows services:

```cpp
std::vector<std::wstring>{
    L"W32Time",
    L"WebClient",
}
```

and requests notifications when either service enters the stopped state:

```cpp
SERVICE_NOTIFY_STOPPED
```

The supplied callback receives the service name and current state and can perform application-specific processing. It must be declared `noexcept`; `Start()` rejects a callable that is not `noexcept` at compile time. The callback owns its own exception safety - `ServiceStatusChangedNotifier` has no way to know what the callback does internally, so a callback whose body can throw should guard itself:

```cpp
[](const std::wstring& service_name, DWORD current_state) noexcept {
  try {
    // Handle the service state change.
  } catch (...) {
    // This callback's own failure, not the notifier's problem.
  }
}
```

The callback executes in the context of the SCM notification mechanism, so it must return quickly. A callback that performs an unbounded network call, waits on a lock that may not become available, or otherwise blocks indefinitely can prevent the notification mechanism from making progress. Offload substantial or potentially blocking work to another thread.

The example logs the notification and demonstrates triggering an application-defined action when a monitored service stops.

## Service Creation and Deletion

Windows also supports SCM-level notifications for service creation and deletion. These notifications can contain multiple service names in a `MULTI_SZ` buffer and use a `/` prefix to distinguish created services from deleted services.

That is a different notification mechanism from the per-service status monitoring implemented here and is not used by the current example.

## Requirements

- A C++23-compatible compiler and standard library.
- Administrator privileges when running the example.

## Build and Run

- [Build on Windows](https://github.com/AmitGDev/cpp-ci-template#build-on-windows)

## Code Style

- [C++](https://github.com/AmitGDev/cpp-coding-standard)
- [Python](https://github.com/AmitGDev/cpp-ci-template#python)
- [CMake](https://github.com/AmitGDev/cpp-ci-template#cmake)

## License

MIT License. Copyright © 2024-2026 Amit Gefen.

---

**Last Updated:** 2026-10-23  
**Maintainer:** AmitGDev
