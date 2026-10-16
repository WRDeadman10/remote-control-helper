#include <windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <random>
#include "SafetyManager.h"
#include "InputInjector.h"
#include "LoopbackServer.h"

static std::atomic<bool> g_keepRunning(true);
static RemoteControl::SafetyManager* g_safetyManagerPtr = nullptr;
static RemoteControl::LoopbackServer* g_serverPtr = nullptr;

BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType) {
    switch (ctrlType) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            std::cout << "\n[MAIN] Termination signal received. Stopping helper..." << std::endl;
            g_keepRunning.store(false);
            if (g_serverPtr) {
                g_serverPtr->Stop();
            }
            if (g_safetyManagerPtr) {
                g_safetyManagerPtr->ReleaseAll();
            }
            return TRUE;
        default:
            return FALSE;
    }
}

static std::string GenerateRandomToken(size_t length = 32) {
    static const char alphabet[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, sizeof(alphabet) - 2);

    std::string token;
    token.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        token += alphabet[dis(gen)];
    }
    return token;
}

int main(int argc, char* argv[]) {
    int port = 8765;
    std::string token;
    std::string tokenFilePath;
    bool requireAuth = true;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--token" && i + 1 < argc) {
            token = argv[++i];
        } else if (arg == "--token-file" && i + 1 < argc) {
            tokenFilePath = argv[++i];
        } else if (arg == "--no-auth") {
            requireAuth = false;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Remote Desktop Control Helper (Windows Native)" << std::endl;
            std::cout << "Usage: remote-control-helper.exe [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --port <port>        Port to listen on (default: 8765)" << std::endl;
            std::cout << "  --token <token>      Authentication token for IPC clients" << std::endl;
            std::cout << "  --token-file <path>  Write authentication token to file" << std::endl;
            std::cout << "  --no-auth            Allow unauthenticated local connections" << std::endl;
            return 0;
        }
    }

    // Auto-generate token if required and none supplied
    if (requireAuth && token.empty()) {
        token = GenerateRandomToken(32);
    }

    if (!tokenFilePath.empty() && !token.empty()) {
        std::ofstream tokenFile(tokenFilePath, std::ios::trunc);
        if (tokenFile.is_open()) {
            tokenFile << token;
            tokenFile.close();
            std::cout << "[MAIN] Wrote auth token to: " << tokenFilePath << std::endl;
        } else {
            std::cerr << "[MAIN] Warning: failed to write token to " << tokenFilePath << std::endl;
        }
    }

    // Initialize Winsock
    WSADATA wsaData;
    int wsaRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaRes != 0) {
        std::cerr << "[MAIN] WSAStartup failed: " << wsaRes << std::endl;
        return 1;
    }

    RemoteControl::SafetyManager safetyManager;
    RemoteControl::InputInjector injector(safetyManager);
    RemoteControl::LoopbackServer server(port, token, requireAuth, injector, safetyManager);

    g_safetyManagerPtr = &safetyManager;
    g_serverPtr = &server;

    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    std::cout << "==========================================================" << std::endl;
    std::cout << "   LAN Camera Remote Desktop Control Helper (Native)      " << std::endl;
    std::cout << "   Listening on: 127.0.0.1:" << port << std::endl;
    std::cout << "   Auth Required: " << (requireAuth ? "YES" : "NO") << std::endl;
    std::cout << "   Emergency Stop: Press Pause/Break or Ctrl+Alt+Shift+Esc" << std::endl;
    std::cout << "==========================================================" << std::endl;

    if (!server.Start()) {
        std::cerr << "[MAIN] Failed to start loopback server. Exiting." << std::endl;
        WSACleanup();
        return 1;
    }

    // Background loop monitoring emergency stop hotkeys
    while (g_keepRunning.load()) {
        Sleep(50);

        // Check for physical emergency stop keys:
        // 1. VK_PAUSE key (Pause/Break key)
        // 2. Ctrl + Alt + Shift + Escape combination
        bool pausePressed = (GetAsyncKeyState(VK_PAUSE) & 0x8000) != 0;
        bool comboPressed = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) &&
                            ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) &&
                            ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) &&
                            ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0);

        if (pausePressed || comboPressed) {
            if (!safetyManager.IsEmergencyStopped()) {
                safetyManager.TriggerEmergencyStop();
                std::cout << "[EMERGENCY STOP] Hardware key triggered emergency stop!" << std::endl;
            }
        }
    }

    server.Stop();
    safetyManager.ReleaseAll();
    WSACleanup();

    std::cout << "[MAIN] Helper exited cleanly." << std::endl;
    return 0;
}
