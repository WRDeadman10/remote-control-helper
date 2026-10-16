#pragma once

#include <windows.h>
#include <string>
#include "SafetyManager.h"
#include "KeycodeMapper.h"

namespace RemoteControl {

class InputInjector {
public:
    explicit InputInjector(SafetyManager& safetyManager);
    ~InputInjector();

    void SetTargetDisplayBounds(int left, int top, int width, int height);

    bool InjectMouseMove(float normX, float normY);
    bool InjectMouseButton(const std::string& button, bool isDown);
    bool InjectMouseClick(const std::string& button);
    bool InjectMouseWheel(int deltaY, int deltaX);

    bool InjectKeyDown(const std::string& domCode);
    bool InjectKeyUp(const std::string& domCode);
    bool InjectKeyTap(const std::string& domCode);

private:
    SafetyManager& m_safetyManager;
    bool m_hasCustomBounds;
    int m_targetLeft;
    int m_targetTop;
    int m_targetWidth;
    int m_targetHeight;

    bool SendMouseInput(DWORD flags, LONG dx, LONG dy, DWORD mouseData);
    bool SendKeyboardInput(WORD vk, bool isExtended, bool isKeyUp);
};

} // namespace RemoteControl
