#include <cassert>
#include <iostream>
#include "KeycodeMapper.h"
#include "InputValidator.h"
#include "SafetyManager.h"
#include "InputInjector.h"

int main() {
    std::cout << "[TEST] Running native remote-control-helper unit tests..." << std::endl;

    // Test 1: Coordinate clamping
    assert(RemoteControl::InputValidator::IsValidCoordinate(0.0f) == true);
    assert(RemoteControl::InputValidator::IsValidCoordinate(1.0f) == true);
    assert(RemoteControl::InputValidator::IsValidCoordinate(0.5f) == true);
    assert(RemoteControl::InputValidator::IsValidCoordinate(-0.01f) == false);
    assert(RemoteControl::InputValidator::IsValidCoordinate(1.01f) == false);
    assert(RemoteControl::InputValidator::ClampCoordinate(-5.0f) == 0.0f);
    assert(RemoteControl::InputValidator::ClampCoordinate(5.0f) == 1.0f);
    assert(RemoteControl::InputValidator::ClampCoordinate(0.75f) == 0.75f);
    std::cout << "  ✔ Coordinate validation & clamping passed." << std::endl;

    // Test 2: Mouse button validation
    assert(RemoteControl::InputValidator::IsValidMouseButton("left") == true);
    assert(RemoteControl::InputValidator::IsValidMouseButton("right") == true);
    assert(RemoteControl::InputValidator::IsValidMouseButton("middle") == true);
    assert(RemoteControl::InputValidator::IsValidMouseButton("invalid") == false);
    std::cout << "  ✔ Mouse button validation passed." << std::endl;

    // Test 3: Wheel clamping
    assert(RemoteControl::InputValidator::ClampWheelDelta(120) == 120);
    assert(RemoteControl::InputValidator::ClampWheelDelta(-120) == -120);
    assert(RemoteControl::InputValidator::ClampWheelDelta(5000) == 1200);
    assert(RemoteControl::InputValidator::ClampWheelDelta(-5000) == -1200);
    std::cout << "  ✔ Mouse wheel clamping passed." << std::endl;

    // Test 4: Keycode mapping
    auto mapA = RemoteControl::KeycodeMapper::MapDomCodeToVirtualKey("KeyA");
    assert(mapA.isValid == true);
    assert(mapA.virtualKey == 'A');
    assert(mapA.isExtended == false);

    auto mapEnter = RemoteControl::KeycodeMapper::MapDomCodeToVirtualKey("Enter");
    assert(mapEnter.isValid == true);
    assert(mapEnter.virtualKey == VK_RETURN);
    assert(mapEnter.isExtended == false);

    auto mapArrowUp = RemoteControl::KeycodeMapper::MapDomCodeToVirtualKey("ArrowUp");
    assert(mapArrowUp.isValid == true);
    assert(mapArrowUp.virtualKey == VK_UP);
    assert(mapArrowUp.isExtended == true);

    auto mapControlRight = RemoteControl::KeycodeMapper::MapDomCodeToVirtualKey("ControlRight");
    assert(mapControlRight.isValid == true);
    assert(mapControlRight.virtualKey == VK_RCONTROL);
    assert(mapControlRight.isExtended == true);

    auto mapPrintScreen = RemoteControl::KeycodeMapper::MapDomCodeToVirtualKey("PrintScreen");
    assert(mapPrintScreen.isValid == true);
    assert(mapPrintScreen.virtualKey == VK_SNAPSHOT);
    assert(mapPrintScreen.isExtended == false);

    auto mapInvalid = RemoteControl::KeycodeMapper::MapDomCodeToVirtualKey("NonExistentKey");
    assert(mapInvalid.isValid == false);
    std::cout << "  ✔ Keycode mapping passed." << std::endl;

    // Test 5: Safety Manager held state & release
    {
        RemoteControl::SafetyManager safety;
        assert(safety.GetHeldKeyCount() == 0);
        assert(safety.GetHeldButtonCount() == 0);
        assert(safety.IsEmergencyStopped() == false);

        safety.RegisterKeyDown('A');
        safety.RegisterKeyDown(VK_LSHIFT);
        safety.RegisterMouseDown(1);
        assert(safety.GetHeldKeyCount() == 2);
        assert(safety.GetHeldButtonCount() == 1);

        safety.RegisterKeyUp('A');
        assert(safety.GetHeldKeyCount() == 1);

        safety.ReleaseAll();
        assert(safety.GetHeldKeyCount() == 0);
        assert(safety.GetHeldButtonCount() == 0);

        // Emergency stop
        safety.TriggerEmergencyStop();
        assert(safety.IsEmergencyStopped() == true);
        safety.ResetEmergencyStop();
        assert(safety.IsEmergencyStopped() == false);
    }
    std::cout << "  ✔ SafetyManager held tracking & emergency stop passed." << std::endl;

    // Test 6: InputInjector Text & Safety
    {
        RemoteControl::SafetyManager safety;
        RemoteControl::InputInjector injector(safety);
        assert(injector.InjectText(L"") == true);
        safety.TriggerEmergencyStop();
        assert(injector.InjectText(L"Test") == false);
        assert(injector.InjectMouseMoveRelative(10, 10) == false);
        safety.ResetEmergencyStop();
    }
    std::cout << "  ✔ InputInjector Unicode text and relative move safety passed." << std::endl;

    std::cout << "[TEST] All native tests passed successfully!" << std::endl;
    return 0;
}
