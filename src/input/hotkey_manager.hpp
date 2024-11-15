#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QMap>
#include <windows.h>
#include <functional>
#include <memory>

namespace whisper_client {
namespace input {

class HotkeyManager : public QObject {
    Q_OBJECT

public:
    explicit HotkeyManager(QObject* parent = nullptr);
    ~HotkeyManager();

    // Hotkey management
    bool setRecordingHotkey(const QString& key);
    bool setActionHotkey(const QString& action, const QString& key);
    void setRecordingMode(const QString& mode);  // "push" or "toggle"
    
    // Start/Stop monitoring
    bool start();
    void stop();

signals:
    void recordingStarted();
    void recordingStopped();
    void actionTriggered(const QString& action);

private:
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static HotkeyManager* instance;  // Needed for the static callback

    bool registerHotkey(const QString& key, int& vkCode);
    void unregisterHotkeys();
    void handleKeyPress(int vkCode);
    void handleKeyRelease(int vkCode);
    QString vkCodeToString(int vkCode);
    int stringToVkCode(const QString& key);

    HHOOK keyboardHook;
    bool isRunning;
    QString recordingMode;  // "push" or "toggle"
    bool isRecording;

    // Hotkey storage
    int recordingKey;
    QMap<QString, int> actionKeys;  // action -> vkCode mapping

    // Windows message handling
    bool nativeEventFilter(const QByteArray& eventType, void* message, long* result);
};

} // namespace input
} // namespace whisper_client