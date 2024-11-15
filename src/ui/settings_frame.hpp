#pragma once

#include <QtWidgets/QFrame>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <memory>

namespace whisper_client {
namespace ui {

class SettingsFrame : public QFrame {
    Q_OBJECT

public:
    explicit SettingsFrame(QWidget *parent = nullptr);
    ~SettingsFrame();

    // Public getters for settings
    QString getSelectedDevice() const;
    QString getSelectedUser() const;
    QString getWebSocketIP() const;
    QString getWebSocketPort() const;
    bool isWebSocketEnabled() const;
    QString getPushToTalkKey() const;
    bool isToggleModeEnabled() const;

public slots:
    void saveSettings();
    void loadSettings();
    void updateDeviceList();

private slots:
    void onWebSocketToggled(bool enabled);
    void onSetHotkeyClicked();
    void onRecordingModeChanged(bool toggleMode);
    void onSetActionHotkeyClicked(const QString& action);
    void onDeviceSelectionChanged(int index);
    void onUserSelectionChanged(int index);

private:
    void setupUi();
    void setupConnections();
    void loadUserNames();
    void createDeviceSection();
    void createUserSection();
    void createWebSocketSection();
    void createHotkeySection();
    void createActionHotkeysSection();
    
    // UI Components
    QVBoxLayout *mainLayout;
    
    // Device selection
    QComboBox *deviceComboBox;
    
    // User selection
    QComboBox *userComboBox;
    
    // WebSocket settings
    QCheckBox *wsEnabledCheckBox;
    QLineEdit *wsIpEdit;
    QLineEdit *wsPortEdit;
    
    // Push to talk settings
    QLineEdit *hotkeyEdit;
    QPushButton *hotkeyButton;
    QCheckBox *recordingModeSwitch;
    
    // Action hotkeys
    struct ActionHotkey {
        QString name;
        QLineEdit *edit;
        QPushButton *button;
    };
    std::vector<ActionHotkey> actionHotkeys;
    
    // Save button
    QPushButton *saveButton;

    bool isSettingHotkey = false;
    QString currentHotkeyTarget;
};

} // namespace ui
} // namespace whisper_client