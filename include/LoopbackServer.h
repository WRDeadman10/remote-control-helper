#pragma once

#include <windows.h>
#include <winsock2.h>
#include <string>
#include <atomic>
#include <thread>
#include "InputInjector.h"
#include "SafetyManager.h"

namespace RemoteControl {

class LoopbackServer {
public:
    LoopbackServer(int port, const std::string& authToken, bool requireAuth,
                   InputInjector& injector, SafetyManager& safetyManager);
    ~LoopbackServer();

    bool Start();
    void Stop();
    bool IsRunning() const;

private:
    int m_port;
    std::string m_authToken;
    bool m_requireAuth;
    InputInjector& m_injector;
    SafetyManager& m_safetyManager;

    SOCKET m_listenSocket;
    std::atomic<bool> m_isRunning;
    std::thread m_serverThread;

    void ServerLoop();
    void HandleClient(SOCKET clientSocket);
    void ProcessMessage(SOCKET clientSocket, const std::string& line, bool& isAuthenticated);

    bool HandleHttpRequest(SOCKET clientSocket, const std::string& request, bool& isWebSocket);
    bool HandleWebSocketHandshake(SOCKET clientSocket, const std::string& request);
    void HandleWebSocketFrames(SOCKET clientSocket, std::string& buffer, bool& isAuthenticated);

    static std::string ExtractStringField(const std::string& json, const std::string& key);
    static float ExtractFloatField(const std::string& json, const std::string& key, float defaultVal = 0.0f);
    static int ExtractIntField(const std::string& json, const std::string& key, int defaultVal = 0);
    static bool ExtractBoolField(const std::string& json, const std::string& key, bool defaultVal = false);
};

} // namespace RemoteControl
