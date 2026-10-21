#include "LoopbackServer.h"
#include <ws2tcpip.h>
#include <wincrypt.h>
#include <iostream>
#include <vector>
#include <sstream>

namespace RemoteControl {

LoopbackServer::LoopbackServer(int port, const std::string& authToken, bool requireAuth,
                               InputInjector& injector, SafetyManager& safetyManager)
    : m_port(port),
      m_authToken(authToken),
      m_requireAuth(requireAuth),
      m_injector(injector),
      m_safetyManager(safetyManager),
      m_listenSocket(INVALID_SOCKET),
      m_isRunning(false) {
}

LoopbackServer::~LoopbackServer() {
    Stop();
}

bool LoopbackServer::Start() {
    if (m_isRunning.load()) {
        return true;
    }

    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET) {
        std::cerr << "[SERVER] Failed to create socket: " << WSAGetLastError() << std::endl;
        return false;
    }

    // Allow address reuse
    int opt = 1;
    setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Strict 127.0.0.1
    serverAddr.sin_port = htons(static_cast<u_short>(m_port));

    if (bind(m_listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[SERVER] Failed to bind to 127.0.0.1:" << m_port << " Error: " << WSAGetLastError() << std::endl;
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        return false;
    }

    if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "[SERVER] Failed to listen on socket. Error: " << WSAGetLastError() << std::endl;
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        return false;
    }

    m_isRunning.store(true);
    std::cout << "[SERVER] Listening on loopback 127.0.0.1:" << m_port << std::endl;

    m_serverThread = std::thread(&LoopbackServer::ServerLoop, this);
    return true;
}

void LoopbackServer::Stop() {
    if (!m_isRunning.exchange(false)) {
        return;
    }

    if (m_listenSocket != INVALID_SOCKET) {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }

    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }

    m_safetyManager.ReleaseAll();
    std::cout << "[SERVER] Stopped." << std::endl;
}

bool LoopbackServer::IsRunning() const {
    return m_isRunning.load();
}

void LoopbackServer::ServerLoop() {
    while (m_isRunning.load()) {
        sockaddr_in clientAddr = {};
        int clientAddrLen = sizeof(clientAddr);

        SOCKET clientSocket = accept(m_listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            if (m_isRunning.load()) {
                std::cerr << "[SERVER] Accept error: " << WSAGetLastError() << std::endl;
            }
            break;
        }

        // Verify connecting client is strictly from loopback
        if (clientAddr.sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
            std::cerr << "[SERVER] Rejected non-loopback connection." << std::endl;
            closesocket(clientSocket);
            continue;
        }

        // Enable TCP_NODELAY for sub-millisecond input dispatch
        int flag = 1;
        setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&flag), sizeof(flag));

        std::thread clientThread(&LoopbackServer::HandleClient, this, clientSocket);
        clientThread.detach();
    }
}

void LoopbackServer::HandleClient(SOCKET clientSocket) {
    bool isAuthenticated = !m_requireAuth;
    bool isWebSocket = false;
    std::string buffer;
    char recvBuffer[4096];

    while (m_isRunning.load()) {
        int bytesRead = recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
        if (bytesRead <= 0) {
            break; // Client disconnected or error
        }

        buffer.append(recvBuffer, bytesRead);

        if (!isWebSocket) {
            // Check if this connection starts with an HTTP request
            if (buffer.rfind("GET ", 0) == 0 || buffer.rfind("POST ", 0) == 0 || buffer.rfind("OPTIONS ", 0) == 0) {
                size_t headerEnd = buffer.find("\r\n\r\n");
                if (headerEnd == std::string::npos) {
                    headerEnd = buffer.find("\n\n");
                }
                if (headerEnd != std::string::npos) {
                    bool keepOpen = HandleHttpRequest(clientSocket, buffer, isWebSocket);
                    if (!keepOpen) {
                        break;
                    }
                    if (isWebSocket) {
                        buffer.erase(0, headerEnd + ((buffer[headerEnd] == '\r') ? 4 : 2));
                        continue;
                    }
                }
                continue;
            }

            // Normal line-delimited JSON
            size_t newlinePos = 0;
            while ((newlinePos = buffer.find('\n')) != std::string::npos) {
                std::string line = buffer.substr(0, newlinePos);
                buffer.erase(0, newlinePos + 1);

                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }

                if (!line.empty()) {
                    ProcessMessage(clientSocket, line, isAuthenticated);
                }
            }
        } else {
            HandleWebSocketFrames(clientSocket, buffer, isAuthenticated);
        }
    }

    // Safety release: ALWAYS release all held keys and mouse buttons when a client disconnects!
    m_safetyManager.ReleaseAll();
    closesocket(clientSocket);
}

bool LoopbackServer::HandleHttpRequest(SOCKET clientSocket, const std::string& request, bool& isWebSocket) {
    // 1. CORS Preflight
    if (request.rfind("OPTIONS ", 0) == 0) {
        std::string response =
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: *\r\n"
            "Access-Control-Allow-Private-Network: true\r\n"
            "Connection: close\r\n\r\n";
        send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);
        return false;
    }

    // 2. WebSocket Upgrade Request
    if (request.find("Upgrade: websocket") != std::string::npos || request.find("upgrade: websocket") != std::string::npos) {
        if (HandleWebSocketHandshake(clientSocket, request)) {
            isWebSocket = true;
            return true;
        }
        return false;
    }

    // 3. Health / Status check (GET /health or GET /status or GET /ping)
    if (request.rfind("GET /health", 0) == 0 || request.rfind("GET /status", 0) == 0 ||
        request.rfind("GET /ping", 0) == 0 || request.rfind("GET / ", 0) == 0) {
        std::string body = "{\"status\":\"ok\",\"running\":true,\"heldKeys\":" +
                           std::to_string(m_safetyManager.GetHeldKeyCount()) + "}\n";
        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Private-Network: true\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " + std::to_string(body.length()) + "\r\n"
            "Connection: close\r\n\r\n" + body;
        send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);
        return false;
    }

    // 4. HTTP POST /input
    if (request.rfind("POST /input", 0) == 0) {
        size_t bodyPos = request.find("\r\n\r\n");
        if (bodyPos != std::string::npos) {
            std::string body = request.substr(bodyPos + 4);
            bool auth = true;
            ProcessMessage(clientSocket, body, auth);
        }
        std::string respBody = "{\"status\":\"ok\"}\n";
        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Private-Network: true\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " + std::to_string(respBody.length()) + "\r\n"
            "Connection: close\r\n\r\n" + respBody;
        send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);
        return false;
    }

    // Default 404
    std::string notFound = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";
    send(clientSocket, notFound.c_str(), static_cast<int>(notFound.length()), 0);
    return false;
}

bool LoopbackServer::HandleWebSocketHandshake(SOCKET clientSocket, const std::string& request) {
    size_t keyPos = request.find("Sec-WebSocket-Key: ");
    if (keyPos == std::string::npos) {
        keyPos = request.find("sec-websocket-key: ");
    }
    if (keyPos == std::string::npos) {
        return false;
    }

    size_t keyStart = keyPos + 19;
    size_t keyEnd = request.find("\r\n", keyStart);
    if (keyEnd == std::string::npos) {
        return false;
    }

    std::string clientKey = request.substr(keyStart, keyEnd - keyStart);
    std::string combined = clientKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BYTE sha1Hash[20] = {0};
    DWORD hashLen = sizeof(sha1Hash);

    if (CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        if (CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
            CryptHashData(hHash, reinterpret_cast<const BYTE*>(combined.c_str()), static_cast<DWORD>(combined.length()), 0);
            CryptGetHashParam(hHash, HP_HASHVAL, sha1Hash, &hashLen, 0);
            CryptDestroyHash(hHash);
        }
        CryptReleaseContext(hProv, 0);
    }

    char base64Buf[64] = {0};
    DWORD base64Len = sizeof(base64Buf);
    CryptBinaryToStringA(sha1Hash, hashLen, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, base64Buf, &base64Len);
    std::string acceptKey(base64Buf);

    std::string response =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: " + acceptKey + "\r\n\r\n";

    send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);
    return true;
}

void LoopbackServer::HandleWebSocketFrames(SOCKET clientSocket, std::string& buffer, bool& isAuthenticated) {
    while (buffer.size() >= 2) {
        const uint8_t* data = reinterpret_cast<const uint8_t*>(buffer.data());
        uint8_t opcode = data[0] & 0x0F;
        bool isMasked = (data[1] & 0x80) != 0;
        uint64_t payloadLen = data[1] & 0x7F;
        size_t headerSize = 2;

        if (payloadLen == 126) {
            if (buffer.size() < 4) {
                return;
            }
            payloadLen = (static_cast<uint64_t>(data[2]) << 8) | data[3];
            headerSize = 4;
        } else if (payloadLen == 127) {
            if (buffer.size() < 10) {
                return;
            }
            payloadLen = 0;
            for (int i = 0; i < 8; ++i) {
                payloadLen = (payloadLen << 8) | data[2 + i];
            }
            headerSize = 10;
        }

        size_t maskOffset = headerSize;
        if (isMasked) {
            headerSize += 4;
        }

        if (buffer.size() < headerSize + payloadLen) {
            return;
        }

        std::string payload;
        payload.resize(static_cast<size_t>(payloadLen));

        if (isMasked) {
            const uint8_t* mask = data + maskOffset;
            const uint8_t* payloadData = data + headerSize;
            for (size_t i = 0; i < payloadLen; ++i) {
                payload[i] = static_cast<char>(payloadData[i] ^ mask[i % 4]);
            }
        } else {
            payload.assign(buffer.data() + headerSize, static_cast<size_t>(payloadLen));
        }

        buffer.erase(0, headerSize + static_cast<size_t>(payloadLen));

        if (opcode == 0x08) {
            // Close frame
            closesocket(clientSocket);
            return;
        } else if (opcode == 0x09) {
            // Ping -> Pong
            uint8_t pongFrame[2] = { 0x8A, 0x00 };
            send(clientSocket, reinterpret_cast<const char*>(pongFrame), 2, 0);
        } else if (opcode == 0x01 || opcode == 0x02) {
            // Text or binary message: parse JSON input event
            if (!payload.empty()) {
                ProcessMessage(clientSocket, payload, isAuthenticated);
            }
        }
    }
}

void LoopbackServer::ProcessMessage(SOCKET clientSocket, const std::string& line, bool& isAuthenticated) {
    std::string type = ExtractStringField(line, "type");
    if (type.empty()) {
        const char* err = "{\"status\":\"error\",\"error\":\"Missing message type\"}\n";
        send(clientSocket, err, static_cast<int>(strlen(err)), 0);
        return;
    }

    // Auth handling
    if (type == "auth") {
        std::string token = ExtractStringField(line, "token");
        if (!m_requireAuth || token == m_authToken) {
            isAuthenticated = true;
            const char* resp = "{\"status\":\"ok\",\"authenticated\":true}\n";
            send(clientSocket, resp, static_cast<int>(strlen(resp)), 0);
        } else {
            const char* resp = "{\"status\":\"error\",\"error\":\"Invalid authentication token\"}\n";
            send(clientSocket, resp, static_cast<int>(strlen(resp)), 0);
        }
        return;
    }

    if (!isAuthenticated) {
        const char* resp = "{\"status\":\"error\",\"error\":\"Unauthorized. Send auth message first.\"}\n";
        send(clientSocket, resp, static_cast<int>(strlen(resp)), 0);
        return;
    }

    if (type == "ping") {
        const char* resp = "{\"status\":\"ok\",\"type\":\"pong\"}\n";
        send(clientSocket, resp, static_cast<int>(strlen(resp)), 0);
        return;
    }

    if (type == "status") {
        std::ostringstream ss;
        ss << "{\"status\":\"ok\",\"emergencyStop\":"
           << (m_safetyManager.IsEmergencyStopped() ? "true" : "false")
           << ",\"heldKeys\":" << m_safetyManager.GetHeldKeyCount()
           << ",\"heldButtons\":" << m_safetyManager.GetHeldButtonCount()
           << "}\n";
        std::string resp = ss.str();
        send(clientSocket, resp.c_str(), static_cast<int>(resp.length()), 0);
        return;
    }

    if (type == "release_all") {
        m_safetyManager.ReleaseAll();
        const char* resp = "{\"status\":\"ok\"}\n";
        send(clientSocket, resp, static_cast<int>(strlen(resp)), 0);
        return;
    }

    if (type == "emergency_stop") {
        m_safetyManager.TriggerEmergencyStop();
        const char* resp = "{\"status\":\"ok\",\"emergencyStop\":true}\n";
        send(clientSocket, resp, static_cast<int>(strlen(resp)), 0);
        return;
    }

    if (type == "reset_emergency_stop") {
        m_safetyManager.ResetEmergencyStop();
        const char* resp = "{\"status\":\"ok\",\"emergencyStop\":false}\n";
        send(clientSocket, resp, static_cast<int>(strlen(resp)), 0);
        return;
    }

    if (type == "set_display_bounds") {
        int left = ExtractIntField(line, "left", 0);
        int top = ExtractIntField(line, "top", 0);
        int width = ExtractIntField(line, "width", 0);
        int height = ExtractIntField(line, "height", 0);
        m_injector.SetTargetDisplayBounds(left, top, width, height);
        const char* resp = "{\"status\":\"ok\"}\n";
        send(clientSocket, resp, static_cast<int>(strlen(resp)), 0);
        return;
    }

    // Input injection commands
    bool success = false;

    if (type == "mouse_move") {
        float x = ExtractFloatField(line, "x", 0.0f);
        float y = ExtractFloatField(line, "y", 0.0f);
        success = m_injector.InjectMouseMove(x, y);
    } else if (type == "mouse_move_relative") {
        int dx = ExtractIntField(line, "dx", 0);
        int dy = ExtractIntField(line, "dy", 0);
        success = m_injector.InjectMouseMoveRelative(dx, dy);
    } else if (type == "mouse_down") {
        std::string button = ExtractStringField(line, "button");
        success = m_injector.InjectMouseButton(button, true);
    } else if (type == "mouse_up") {
        std::string button = ExtractStringField(line, "button");
        success = m_injector.InjectMouseButton(button, false);
    } else if (type == "mouse_click") {
        std::string button = ExtractStringField(line, "button");
        success = m_injector.InjectMouseClick(button);
    } else if (type == "mouse_wheel") {
        int deltaY = ExtractIntField(line, "deltaY", 0);
        int deltaX = ExtractIntField(line, "deltaX", 0);
        success = m_injector.InjectMouseWheel(deltaY, deltaX);
    } else if (type == "key_down") {
        std::string code = ExtractStringField(line, "code");
        success = m_injector.InjectKeyDown(code);
    } else if (type == "key_up") {
        std::string code = ExtractStringField(line, "code");
        success = m_injector.InjectKeyUp(code);
    } else if (type == "key_tap") {
        std::string code = ExtractStringField(line, "code");
        success = m_injector.InjectKeyTap(code);
    } else if (type == "text") {
        std::string utf8Text = ExtractStringField(line, "text");
        if (utf8Text.empty()) {
            success = true;
        } else {
            int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8Text.c_str(), -1, NULL, 0);
            if (wlen > 1) {
                std::vector<wchar_t> wstr(wlen);
                MultiByteToWideChar(CP_UTF8, 0, utf8Text.c_str(), -1, wstr.data(), wlen);
                std::wstring textStr(wstr.data());
                success = m_injector.InjectText(textStr);
            } else {
                success = true;
            }
        }
    } else {
        const char* resp = "{\"status\":\"error\",\"error\":\"Unknown command\"}\n";
        send(clientSocket, resp, static_cast<int>(strlen(resp)), 0);
        return;
    }

    if (success) {
        const char* resp = "{\"status\":\"ok\"}\n";
        send(clientSocket, resp, static_cast<int>(strlen(resp)), 0);
    } else {
        const char* resp = "{\"status\":\"error\",\"error\":\"Injection failed or rejected\"}\n";
        send(clientSocket, resp, static_cast<int>(strlen(resp)), 0);
    }
}

std::string LoopbackServer::ExtractStringField(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    size_t keyPos = json.find(needle);
    if (keyPos == std::string::npos) {
        return "";
    }

    size_t colonPos = json.find(':', keyPos + needle.length());
    if (colonPos == std::string::npos) {
        return "";
    }

    size_t startQuote = json.find('\"', colonPos);
    if (startQuote == std::string::npos) {
        return "";
    }

    size_t endQuote = json.find('\"', startQuote + 1);
    if (endQuote == std::string::npos) {
        return "";
    }

    return json.substr(startQuote + 1, endQuote - startQuote - 1);
}

float LoopbackServer::ExtractFloatField(const std::string& json, const std::string& key, float defaultVal) {
    std::string needle = "\"" + key + "\"";
    size_t keyPos = json.find(needle);
    if (keyPos == std::string::npos) {
        return defaultVal;
    }

    size_t colonPos = json.find(':', keyPos + needle.length());
    if (colonPos == std::string::npos) {
        return defaultVal;
    }

    size_t start = json.find_first_of("-0123456789.", colonPos);
    if (start == std::string::npos) {
        return defaultVal;
    }

    size_t end = json.find_first_not_of("-0123456789.eE", start);
    std::string numStr = (end == std::string::npos) ? json.substr(start) : json.substr(start, end - start);

    try {
        return std::stof(numStr);
    } catch (...) {
        return defaultVal;
    }
}

int LoopbackServer::ExtractIntField(const std::string& json, const std::string& key, int defaultVal) {
    float f = ExtractFloatField(json, key, static_cast<float>(defaultVal));
    return static_cast<int>(f);
}

bool LoopbackServer::ExtractBoolField(const std::string& json, const std::string& key, bool defaultVal) {
    std::string needle = "\"" + key + "\"";
    size_t keyPos = json.find(needle);
    if (keyPos == std::string::npos) {
        return defaultVal;
    }

    size_t colonPos = json.find(':', keyPos + needle.length());
    if (colonPos == std::string::npos) {
        return defaultVal;
    }

    if (json.find("true", colonPos) != std::string::npos) {
        return true;
    }
    if (json.find("false", colonPos) != std::string::npos) {
        return false;
    }
    return defaultVal;
}

} // namespace RemoteControl
