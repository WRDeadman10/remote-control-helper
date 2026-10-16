#pragma once

#include <string>
#include <algorithm>

namespace RemoteControl {

class InputValidator {
public:
    static bool IsValidCoordinate(float val) {
        return val >= 0.0f && val <= 1.0f;
    }

    static float ClampCoordinate(float val) {
        if (val < 0.0f) {
            return 0.0f;
        }
        if (val > 1.0f) {
            return 1.0f;
        }
        return val;
    }

    static bool IsValidMouseButton(const std::string& button) {
        return button == "left" || button == "right" || button == "middle" ||
               button == "mouse4" || button == "mouse5";
    }

    static int ClampWheelDelta(int delta) {
        const int maxDelta = 1200;
        if (delta > maxDelta) {
            return maxDelta;
        }
        if (delta < -maxDelta) {
            return -maxDelta;
        }
        return delta;
    }
};

} // namespace RemoteControl
