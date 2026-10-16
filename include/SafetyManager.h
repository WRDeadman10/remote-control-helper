#pragma once

#include <windows.h>
#include <set>
#include <atomic>
#include <mutex>

namespace RemoteControl {

class SafetyManager {
public:
    SafetyManager();
    ~SafetyManager();

    void RegisterKeyDown(WORD vk);
    void RegisterKeyUp(WORD vk);

    void RegisterMouseDown(int button);
    void RegisterMouseUp(int button);

    void ReleaseAll();

    void TriggerEmergencyStop();
    void ResetEmergencyStop();
    bool IsEmergencyStopped() const;

    size_t GetHeldKeyCount() const;
    size_t GetHeldButtonCount() const;

private:
    std::set<WORD> m_heldKeys;
    std::set<int> m_heldMouseButtons;
    mutable std::mutex m_mutex;
    std::atomic<bool> m_isEmergencyStopped;
};

} // namespace RemoteControl
