#pragma once

#include <windows.h>
#include <string>

namespace RemoteControl {

struct KeyMapping {
    WORD virtualKey;
    bool isExtended;
    bool isValid;
};

class KeycodeMapper {
public:
    static KeyMapping MapDomCodeToVirtualKey(const std::string& domCode);
};

} // namespace RemoteControl
