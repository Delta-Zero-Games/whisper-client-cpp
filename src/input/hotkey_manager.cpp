#include "input/hotkey_manager.hpp"
#include <QtCore/QDebug>
#include <QtCore/QDateTime>

namespace whisper_client {
namespace input {

HotkeyManager* HotkeyManager::instance = nullptr;

HotkeyManager::HotkeyManager(QObject* parent)
    : QObject(parent)
    , keyboardHook(nullptr)
    , isRunning(false)
    , recordingMode("push")
    , isRecording(false)
    , recordingKey(0)
{
    instance = this;
}

HotkeyManager::~HotkeyManager() {
    stop();
}

bool HotkeyManager::start() {
    if (isRunning) return true;

    // Install the keyboard hook
    keyboardHook = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        LowLevelKeyboardProc,
        GetModuleHandle(nullptr),
        0
    );

    if (!keyboardHook) {
        qWarning() << "Failed to install keyboard hook. Error:" << GetLastError();
        return false;
    }

    isRunning = true;
    qDebug() << "Hotkey manager started";
    return true;
}

void HotkeyManager::stop() {
    if (!isRunning) return;

    if (keyboardHook) {
        UnhookWindowsHookEx(keyboardHook);
        keyboardHook = nullptr;
    }

    isRunning = false;
    qDebug() << "Hotkey manager stopped";
}

bool HotkeyManager::setRecordingHotkey(const QString& key) {
    int vkCode = stringToVkCode(key);
    if (vkCode == 0) {
        qWarning() << "Invalid recording hotkey:" << key;
        return false;
    }

    recordingKey = vkCode;
    qDebug() << "Recording hotkey set to:" << key;
    return true;
}

bool HotkeyManager::setActionHotkey(const QString& action, const QString& key) {
    int vkCode = stringToVkCode(key);
    if (vkCode == 0) {
        qWarning() << "Invalid action hotkey:" << key << "for action:" << action;
        return false;
    }

    actionKeys[action] = vkCode;
    qDebug() << "Action hotkey set for" << action << ":" << key;
    return true;
}

void HotkeyManager::setRecordingMode(const QString& mode) {
    if (mode == "push" || mode == "toggle") {
        recordingMode = mode;
        qDebug() << "Recording mode set to:" << mode;
    }
}

LRESULT CALLBACK HotkeyManager::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && instance) {
        KBDLLHOOKSTRUCT* kbd = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            instance->handleKeyPress(kbd->vkCode);
        }
        else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            instance->handleKeyRelease(kbd->vkCode);
        }
    }

    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void HotkeyManager::handleKeyPress(int vkCode) {
    // Handle recording hotkey
    if (vkCode == recordingKey) {
        if (recordingMode == "push") {
            if (!isRecording) {
                isRecording = true;
                emit recordingStarted();
            }
        }
        else if (recordingMode == "toggle") {
            isRecording = !isRecording;
            if (isRecording) {
                emit recordingStarted();
            } else {
                emit recordingStopped();
            }
        }
    }

    // Handle action hotkeys
    for (auto it = actionKeys.begin(); it != actionKeys.end(); ++it) {
        if (it.value() == vkCode) {
            emit actionTriggered(it.key());
            break;
        }
    }
}

void HotkeyManager::handleKeyRelease(int vkCode) {
    if (vkCode == recordingKey && recordingMode == "push") {
        if (isRecording) {
            isRecording = false;
            emit recordingStopped();
        }
    }
}

int HotkeyManager::stringToVkCode(const QString& key) {
    // Handle function keys
    if (key.startsWith("f", Qt::CaseInsensitive) && key.length() <= 3) {
        bool ok;
        int num = key.midRef(1).toInt(&ok);
        if (ok && num >= 1 && num <= 24) {
            return VK_F1 + (num - 1);
        }
    }

    // Handle other special keys
    static const QMap<QString, int> specialKeys = {
        {"alt", VK_MENU},
        {"ctrl", VK_CONTROL},
        {"shift", VK_SHIFT},
        {"space", VK_SPACE},
        {"tab", VK_TAB},
        {"enter", VK_RETURN},
        {"escape", VK_ESCAPE},
        {"esc", VK_ESCAPE}
    };

    if (specialKeys.contains(key.toLower())) {
        return specialKeys[key.toLower()];
    }

    // Handle single characters
    if (key.length() == 1) {
        return key[0].toUpper().unicode();
    }

    return 0;
}

QString HotkeyManager::vkCodeToString(int vkCode) {
    // Handle function keys
    if (vkCode >= VK_F1 && vkCode <= VK_F24) {
        return QString("F%1").arg(vkCode - VK_F1 + 1);
    }

    // Handle other special keys
    static const QMap<int, QString> specialKeys = {
        {VK_MENU, "Alt"},
        {VK_CONTROL, "Ctrl"},
        {VK_SHIFT, "Shift"},
        {VK_SPACE, "Space"},
        {VK_TAB, "Tab"},
        {VK_RETURN, "Enter"},
        {VK_ESCAPE, "Escape"}
    };

    if (specialKeys.contains(vkCode)) {
        return specialKeys[vkCode];
    }

    // Handle regular characters
    if (vkCode >= 'A' && vkCode <= 'Z') {
        return QString(QChar(vkCode));
    }

    return QString();
}

} // namespace input
} // namespace whisper_client