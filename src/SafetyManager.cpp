#include "SafetyManager.h"
#include <vector>
#include <iostream>

namespace RemoteControl {

SafetyManager::SafetyManager()
    : m_isEmergencyStopped(false) {
}

SafetyManager::~SafetyManager() {
    ReleaseAll();
}

void SafetyManager::RegisterKeyDown(WORD vk) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_heldKeys.insert(vk);
}

void SafetyManager::RegisterKeyUp(WORD vk) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_heldKeys.erase(vk);
}

void SafetyManager::RegisterMouseDown(int button) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_heldMouseButtons.insert(button);
}

void SafetyManager::RegisterMouseUp(int button) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_heldMouseButtons.erase(button);
}

void SafetyManager::ReleaseAll() {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<INPUT> inputs;

    // Release all held mouse buttons
    for (int button : m_heldMouseButtons) {
        INPUT input = {};
        input.type = INPUT_MOUSE;

        if (button == 1) { // Left
            input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            inputs.push_back(input);
        } else if (button == 2) { // Right
            input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
            inputs.push_back(input);
        } else if (button == 3) { // Middle
            input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
            inputs.push_back(input);
        }
    }
    m_heldMouseButtons.clear();

    // Release all held keyboard keys
    for (WORD vk : m_heldKeys) {
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vk;
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        inputs.push_back(input);
    }
    m_heldKeys.clear();

    if (!inputs.empty()) {
        UINT sent = SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
        (void)sent;
    }
}

void SafetyManager::TriggerEmergencyStop() {
    m_isEmergencyStopped.store(true);
    ReleaseAll();
    std::cout << "[SAFETY] Emergency stop triggered! All inputs released and locked." << std::endl;
}

void SafetyManager::ResetEmergencyStop() {
    m_isEmergencyStopped.store(false);
    std::cout << "[SAFETY] Emergency stop reset. Input accepted." << std::endl;
}

bool SafetyManager::IsEmergencyStopped() const {
    return m_isEmergencyStopped.load();
}

size_t SafetyManager::GetHeldKeyCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_heldKeys.size();
}

size_t SafetyManager::GetHeldButtonCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_heldMouseButtons.size();
}

} // namespace RemoteControl
