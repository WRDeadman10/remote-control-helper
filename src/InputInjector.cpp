#include "InputInjector.h"
#include "InputValidator.h"
#include <iostream>

namespace RemoteControl {

InputInjector::InputInjector(SafetyManager& safetyManager)
    : m_safetyManager(safetyManager),
      m_hasCustomBounds(false),
      m_targetLeft(0),
      m_targetTop(0),
      m_targetWidth(0),
      m_targetHeight(0) {
}

InputInjector::~InputInjector() {
}

void InputInjector::SetTargetDisplayBounds(int left, int top, int width, int height) {
    if (width > 0 && height > 0) {
        m_hasCustomBounds = true;
        m_targetLeft = left;
        m_targetTop = top;
        m_targetWidth = width;
        m_targetHeight = height;
    } else {
        m_hasCustomBounds = false;
    }
}

bool InputInjector::SendMouseInput(DWORD flags, LONG dx, LONG dy, DWORD mouseData) {
    if (m_safetyManager.IsEmergencyStopped()) {
        return false;
    }

    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.mouseData = mouseData;
    input.mi.dwFlags = flags;
    input.mi.time = 0;
    input.mi.dwExtraInfo = 0;

    UINT sent = SendInput(1, &input, sizeof(INPUT));
    return sent == 1;
}

bool InputInjector::SendKeyboardInput(WORD vk, bool isExtended, bool isKeyUp) {
    if (m_safetyManager.IsEmergencyStopped()) {
        return false;
    }

    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.wScan = static_cast<WORD>(MapVirtualKeyW(vk, MAPVK_VK_TO_VSC));
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;

    DWORD flags = 0;
    if (isExtended) {
        flags |= KEYEVENTF_EXTENDEDKEY;
    }
    if (isKeyUp) {
        flags |= KEYEVENTF_KEYUP;
    }
    input.ki.dwFlags = flags;

    UINT sent = SendInput(1, &input, sizeof(INPUT));
    return sent == 1;
}

bool InputInjector::InjectMouseMove(float normX, float normY) {
    normX = InputValidator::ClampCoordinate(normX);
    normY = InputValidator::ClampCoordinate(normY);

    int vLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int vTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int vWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int vHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    if (vWidth <= 0 || vHeight <= 0) {
        vWidth = GetSystemMetrics(SM_CXSCREEN);
        vHeight = GetSystemMetrics(SM_CYSCREEN);
        vLeft = 0;
        vTop = 0;
    }

    int pixelX = 0;
    int pixelY = 0;

    if (m_hasCustomBounds && m_targetWidth > 0 && m_targetHeight > 0) {
        pixelX = m_targetLeft + static_cast<int>(normX * static_cast<float>(m_targetWidth));
        pixelY = m_targetTop + static_cast<int>(normY * static_cast<float>(m_targetHeight));
    } else {
        // Default to primary display
        int primaryWidth = GetSystemMetrics(SM_CXSCREEN);
        int primaryHeight = GetSystemMetrics(SM_CYSCREEN);
        pixelX = static_cast<int>(normX * static_cast<float>(primaryWidth));
        pixelY = static_cast<int>(normY * static_cast<float>(primaryHeight));
    }

    // Convert pixel coordinates to normalized virtual screen coordinates (0 to 65535)
    LONG normDx = static_cast<LONG>(((static_cast<float>(pixelX - vLeft) * 65535.0f) / static_cast<float>(vWidth)));
    LONG normDy = static_cast<LONG>(((static_cast<float>(pixelY - vTop) * 65535.0f) / static_cast<float>(vHeight)));

    DWORD flags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
    return SendMouseInput(flags, normDx, normDy, 0);
}

bool InputInjector::InjectMouseMoveRelative(int dx, int dy) {
    if (m_safetyManager.IsEmergencyStopped()) {
        return false;
    }
    DWORD flags = MOUSEEVENTF_MOVE;
    return SendMouseInput(flags, static_cast<LONG>(dx), static_cast<LONG>(dy), 0);
}

bool InputInjector::InjectMouseButton(const std::string& button, bool isDown) {
    DWORD flags = 0;
    int buttonCode = 0;

    if (button == "left") {
        buttonCode = 1;
        flags = isDown ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    } else if (button == "right") {
        buttonCode = 2;
        flags = isDown ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    } else if (button == "middle") {
        buttonCode = 3;
        flags = isDown ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
    } else {
        return false;
    }

    if (isDown) {
        m_safetyManager.RegisterMouseDown(buttonCode);
    } else {
        m_safetyManager.RegisterMouseUp(buttonCode);
    }

    return SendMouseInput(flags, 0, 0, 0);
}

bool InputInjector::InjectMouseClick(const std::string& button) {
    bool downSuccess = InjectMouseButton(button, true);
    if (!downSuccess) {
        return false;
    }
    Sleep(5);
    return InjectMouseButton(button, false);
}

bool InputInjector::InjectMouseWheel(int deltaY, int deltaX) {
    bool ok = true;
    if (deltaY != 0) {
        int clampedY = InputValidator::ClampWheelDelta(deltaY);
        bool yRes = SendMouseInput(MOUSEEVENTF_WHEEL, 0, 0, static_cast<DWORD>(clampedY));
        ok = ok && yRes;
    }
    if (deltaX != 0) {
        int clampedX = InputValidator::ClampWheelDelta(deltaX);
        bool xRes = SendMouseInput(MOUSEEVENTF_HWHEEL, 0, 0, static_cast<DWORD>(clampedX));
        ok = ok && xRes;
    }
    return ok;
}

bool InputInjector::InjectKeyDown(const std::string& domCode) {
    KeyMapping mapping = KeycodeMapper::MapDomCodeToVirtualKey(domCode);
    if (!mapping.isValid) {
        return false;
    }

    m_safetyManager.RegisterKeyDown(mapping.virtualKey);
    return SendKeyboardInput(mapping.virtualKey, mapping.isExtended, false);
}

bool InputInjector::InjectKeyUp(const std::string& domCode) {
    KeyMapping mapping = KeycodeMapper::MapDomCodeToVirtualKey(domCode);
    if (!mapping.isValid) {
        return false;
    }

    m_safetyManager.RegisterKeyUp(mapping.virtualKey);
    return SendKeyboardInput(mapping.virtualKey, mapping.isExtended, true);
}

bool InputInjector::InjectKeyTap(const std::string& domCode) {
    bool downSuccess = InjectKeyDown(domCode);
    if (!downSuccess) {
        return false;
    }
    Sleep(5);
    return InjectKeyUp(domCode);
}

bool InputInjector::InjectText(const std::wstring& text) {
    if (m_safetyManager.IsEmergencyStopped()) {
        return false;
    }
    if (text.empty()) {
        return true;
    }

    bool allOk = true;
    for (wchar_t ch : text) {
        INPUT inputs[2] = {};
        // Key down (Unicode character)
        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = 0;
        inputs[0].ki.wScan = ch;
        inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;
        inputs[0].ki.time = 0;
        inputs[0].ki.dwExtraInfo = 0;

        // Key up (Unicode character)
        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = 0;
        inputs[1].ki.wScan = ch;
        inputs[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
        inputs[1].ki.time = 0;
        inputs[1].ki.dwExtraInfo = 0;

        UINT sent = SendInput(2, inputs, sizeof(INPUT));
        if (sent != 2) {
            allOk = false;
        }
    }
    return allOk;
}

} // namespace RemoteControl
