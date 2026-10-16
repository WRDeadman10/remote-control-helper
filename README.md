# Remote Control Helper (Windows Native)

Native C++ Windows helper application providing low-latency loopback remote desktop control for the LAN Camera Streaming ecosystem.

## Responsibilities

- Mouse movement and button injection via Windows `SendInput`.
- Mouse-wheel input (vertical and horizontal delta).
- Keyboard input with DOM code to Virtual-Key code mapping and extended key support.
- Loopback TCP server on `127.0.0.1:8765` with authentication handshake token.
- Held-input registry (`SafetyManager`) with automatic safety release on client disconnect.
- Hardware emergency stop (`Pause/Break` or `Ctrl+Alt+Shift+Esc`).

## Building

Requires CMake 3.15+ and Visual Studio 2019/2022 (MSVC).

```cmd
cmake -B build
cmake --build build --config Release
```

The resulting executable is generated at `build/Release/remote-control-helper.exe`.

## Running

```cmd
build\Release\remote-control-helper.exe --port 8765 --no-auth
```

Or with auto-generated token file:

```cmd
build\Release\remote-control-helper.exe --port 8765 --token-file auth.token
```

## Running Tests

```cmd
build\Release\helper-tests.exe
```
