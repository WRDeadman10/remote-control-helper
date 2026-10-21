#include "KeycodeMapper.h"
#include <unordered_map>

namespace RemoteControl {

KeyMapping KeycodeMapper::MapDomCodeToVirtualKey(const std::string& domCode) {
    static const std::unordered_map<std::string, KeyMapping> keyTable = {
        // Letters
        {"KeyA", { 'A', false, true }},
        {"KeyB", { 'B', false, true }},
        {"KeyC", { 'C', false, true }},
        {"KeyD", { 'D', false, true }},
        {"KeyE", { 'E', false, true }},
        {"KeyF", { 'F', false, true }},
        {"KeyG", { 'G', false, true }},
        {"KeyH", { 'H', false, true }},
        {"KeyI", { 'I', false, true }},
        {"KeyJ", { 'J', false, true }},
        {"KeyK", { 'K', false, true }},
        {"KeyL", { 'L', false, true }},
        {"KeyM", { 'M', false, true }},
        {"KeyN", { 'N', false, true }},
        {"KeyO", { 'O', false, true }},
        {"KeyP", { 'P', false, true }},
        {"KeyQ", { 'Q', false, true }},
        {"KeyR", { 'R', false, true }},
        {"KeyS", { 'S', false, true }},
        {"KeyT", { 'T', false, true }},
        {"KeyU", { 'U', false, true }},
        {"KeyV", { 'V', false, true }},
        {"KeyW", { 'W', false, true }},
        {"KeyX", { 'X', false, true }},
        {"KeyY", { 'Y', false, true }},
        {"KeyZ", { 'Z', false, true }},

        // Top-row Digits
        {"Digit0", { '0', false, true }},
        {"Digit1", { '1', false, true }},
        {"Digit2", { '2', false, true }},
        {"Digit3", { '3', false, true }},
        {"Digit4", { '4', false, true }},
        {"Digit5", { '5', false, true }},
        {"Digit6", { '6', false, true }},
        {"Digit7", { '7', false, true }},
        {"Digit8", { '8', false, true }},
        {"Digit9", { '9', false, true }},

        // Function Keys
        {"F1",  { VK_F1,  false, true }},
        {"F2",  { VK_F2,  false, true }},
        {"F3",  { VK_F3,  false, true }},
        {"F4",  { VK_F4,  false, true }},
        {"F5",  { VK_F5,  false, true }},
        {"F6",  { VK_F6,  false, true }},
        {"F7",  { VK_F7,  false, true }},
        {"F8",  { VK_F8,  false, true }},
        {"F9",  { VK_F9,  false, true }},
        {"F10", { VK_F10, false, true }},
        {"F11", { VK_F11, false, true }},
        {"F12", { VK_F12, false, true }},

        // Navigation & Editing
        {"Escape",     { VK_ESCAPE, false, true }},
        {"Space",      { VK_SPACE,  false, true }},
        {"Enter",      { VK_RETURN, false, true }},
        {"NumpadEnter",{ VK_RETURN, true,  true }},
        {"Backspace",  { VK_BACK,   false, true }},
        {"Tab",        { VK_TAB,    false, true }},
        {"CapsLock",   { VK_CAPITAL,false, true }},
        {"NumLock",    { VK_NUMLOCK,true,  true }},
        {"ScrollLock", { VK_SCROLL, false, true }},
        {"PrintScreen",{ VK_SNAPSHOT, false, true }},
        {"Pause",      { VK_PAUSE,  true,  true }},

        // Extended Navigation Cluster
        {"Insert",     { VK_INSERT, true,  true }},
        {"Delete",     { VK_DELETE, true,  true }},
        {"Home",       { VK_HOME,   true,  true }},
        {"End",        { VK_END,    true,  true }},
        {"PageUp",     { VK_PRIOR,  true,  true }},
        {"PageDown",   { VK_NEXT,   true,  true }},

        // Arrow Keys
        {"ArrowUp",    { VK_UP,     true,  true }},
        {"ArrowDown",  { VK_DOWN,   true,  true }},
        {"ArrowLeft",  { VK_LEFT,   true,  true }},
        {"ArrowRight", { VK_RIGHT,  true,  true }},

        // Modifiers
        {"ShiftLeft",   { VK_LSHIFT,   false, true }},
        {"ShiftRight",  { VK_RSHIFT,   false, true }},
        {"ControlLeft", { VK_LCONTROL, false, true }},
        {"ControlRight",{ VK_RCONTROL, true,  true }},
        {"AltLeft",     { VK_LMENU,    false, true }},
        {"AltRight",    { VK_RMENU,    true,  true }},
        {"MetaLeft",    { VK_LWIN,     false, true }},
        {"MetaRight",   { VK_RWIN,     false, true }},
        {"ContextMenu", { VK_APPS,     true,  true }},

        // Numpad
        {"Numpad0",        { VK_NUMPAD0,  false, true }},
        {"Numpad1",        { VK_NUMPAD1,  false, true }},
        {"Numpad2",        { VK_NUMPAD2,  false, true }},
        {"Numpad3",        { VK_NUMPAD3,  false, true }},
        {"Numpad4",        { VK_NUMPAD4,  false, true }},
        {"Numpad5",        { VK_NUMPAD5,  false, true }},
        {"Numpad6",        { VK_NUMPAD6,  false, true }},
        {"Numpad7",        { VK_NUMPAD7,  false, true }},
        {"Numpad8",        { VK_NUMPAD8,  false, true }},
        {"Numpad9",        { VK_NUMPAD9,  false, true }},
        {"NumpadAdd",      { VK_ADD,      false, true }},
        {"NumpadSubtract", { VK_SUBTRACT, false, true }},
        {"NumpadMultiply", { VK_MULTIPLY, false, true }},
        {"NumpadDivide",   { VK_DIVIDE,   true,  true }},
        {"NumpadDecimal",  { VK_DECIMAL,  false, true }},

        // Punctuation & OEM Keys
        {"Semicolon",    { VK_OEM_1,      false, true }},
        {"Equal",        { VK_OEM_PLUS,   false, true }},
        {"Comma",        { VK_OEM_COMMA,  false, true }},
        {"Minus",        { VK_OEM_MINUS,  false, true }},
        {"Period",       { VK_OEM_PERIOD, false, true }},
        {"Slash",        { VK_OEM_2,      false, true }},
        {"Backquote",    { VK_OEM_3,      false, true }},
        {"BracketLeft",  { VK_OEM_4,      false, true }},
        {"Backslash",    { VK_OEM_5,      false, true }},
        {"BracketRight", { VK_OEM_6,      false, true }},
        {"Quote",        { VK_OEM_7,      false, true }}
    };

    auto it = keyTable.find(domCode);
    if (it != keyTable.end()) {
        return it->second;
    }

    return { 0, false, false };
}

} // namespace RemoteControl
