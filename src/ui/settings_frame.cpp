#include "ui/settings_frame.hpp"
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QGridLayout>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtWidgets/QFileDialog>

namespace whisper_client {
namespace ui {

SettingsFrame::SettingsFrame(QWidget *parent)
    : QFrame(parent)
{
    setupUi();
    setupConnections();
    loadSettings();
}

SettingsFrame::~SettingsFrame() = default;

void SettingsFrame::setupUi() {
    // Main layout
    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    // Create sections
    createDeviceSection();
    createUserSection();
    createWebSocketSection();
    createHotkeySection();
    createActionHotkeysSection();

    // Save button
    saveButton = new QPushButton("Save Settings", this);
    mainLayout->addWidget(saveButton);
}

void SettingsFrame::createDeviceSection() {
    auto *deviceGroup = new QGroupBox("Microphone", this);
    auto *deviceLayout = new QVBoxLayout(deviceGroup);
    
    deviceComboBox = new QComboBox(this);
    deviceLayout->addWidget(deviceComboBox);
    
    mainLayout->addWidget(deviceGroup);
    updateDeviceList();
}

void SettingsFrame::createUserSection() {
    auto *userGroup = new QGroupBox("User", this);
    auto *userLayout = new QVBoxLayout(userGroup);
    
    userComboBox = new QComboBox(this);
    userLayout->addWidget(userComboBox);
    
    mainLayout->addWidget(userGroup);
    loadUserNames();
}

void SettingsFrame::createWebSocketSection() {
    auto *wsGroup = new QGroupBox("WebSocket", this);
    auto *wsLayout = new QVBoxLayout(wsGroup);
    
    // Enable checkbox
    wsEnabledCheckBox = new QCheckBox("Enable WebSocket", this);
    wsLayout->addWidget(wsEnabledCheckBox);
    
    // IP and Port fields
    auto *wsAddressLayout = new QHBoxLayout();
    wsIpEdit = new QLineEdit(this);
    wsIpEdit->setPlaceholderText("IP Address");
    wsPortEdit = new QLineEdit(this);
    wsPortEdit->setPlaceholderText("Port");
    wsPortEdit->setMaximumWidth(100);
    
    wsAddressLayout->addWidget(wsIpEdit);
    wsAddressLayout->addWidget(wsPortEdit);
    wsLayout->addLayout(wsAddressLayout);
    
    mainLayout->addWidget(wsGroup);
}

void SettingsFrame::createHotkeySection() {
    auto *hotkeyGroup = new QGroupBox("Push to Talk", this);
    auto *hotkeyLayout = new QHBoxLayout(hotkeyGroup);
    
    hotkeyEdit = new QLineEdit(this);
    hotkeyEdit->setReadOnly(true);
    hotkeyButton = new QPushButton("Set Key", this);
    recordingModeSwitch = new QCheckBox("Toggle Mode", this);
    
    hotkeyLayout->addWidget(hotkeyEdit);
    hotkeyLayout->addWidget(hotkeyButton);
    hotkeyLayout->addWidget(recordingModeSwitch);
    
    mainLayout->addWidget(hotkeyGroup);
}

void SettingsFrame::createActionHotkeysSection() {
    auto *actionGroup = new QGroupBox("Twitch Action Hotkeys", this);
    auto *actionLayout = new QGridLayout(actionGroup);
    
    const QStringList actions = {"TTS", "Follows", "Subs", "Gifts"};
    
    for (int i = 0; i < actions.size(); ++i) {
        const QString &action = actions[i];
        
        auto *edit = new QLineEdit(this);
        edit->setReadOnly(true);
        
        auto *button = new QPushButton("Set " + action + " Key", this);
        
        actionLayout->addWidget(edit, i, 0);
        actionLayout->addWidget(button, i, 1);
        
        actionHotkeys.push_back({action.toLower(), edit, button});
        
        connect(button, &QPushButton::clicked, this, 
            [this, action](){ onSetActionHotkeyClicked(action.toLower()); });
    }
    
    mainLayout->addWidget(actionGroup);
}

void SettingsFrame::setupConnections() {
    connect(wsEnabledCheckBox, &QCheckBox::toggled, this, &SettingsFrame::onWebSocketToggled);
    connect(hotkeyButton, &QPushButton::clicked, this, &SettingsFrame::onSetHotkeyClicked);
    connect(recordingModeSwitch, &QCheckBox::toggled, this, &SettingsFrame::onRecordingModeChanged);
    connect(saveButton, &QPushButton::clicked, this, &SettingsFrame::saveSettings);
    connect(deviceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsFrame::onDeviceSelectionChanged);
    connect(userComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsFrame::onUserSelectionChanged);
}

void SettingsFrame::updateDeviceList() {
    deviceComboBox->clear();
    // TODO: Implement audio device listing using RtAudio
    deviceComboBox->addItem("Default Device"); // Placeholder
}

void SettingsFrame::loadUserNames() {
    QFile file("user_names.json");
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject obj = doc.object();
        QJsonArray names = obj["names"].toArray();
        
        userComboBox->clear();
        for (const auto &name : names) {
            userComboBox->addItem(name.toString());
        }
    } else {
        userComboBox->addItem("Default");
    }
}

void SettingsFrame::loadSettings() {
    QFile file("config.json");
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject config = doc.object();
        
        // Load WebSocket settings
        wsEnabledCheckBox->setChecked(config["ws_enabled"].toBool(true));
        wsIpEdit->setText(config["ws_ip"].toString("localhost"));
        wsPortEdit->setText(config["ws_port"].toString("3001"));
        
        // Load push-to-talk settings
        hotkeyEdit->setText(config["push_to_talk_key"].toString("f5"));
        recordingModeSwitch->setChecked(config["recording_mode"].toString("push") == "toggle");
        
        // Load action hotkeys
        for (auto &hotkey : actionHotkeys) {
            QString key = hotkey.name + "_hotkey";
            hotkey.edit->setText(config[key].toString());
        }
        
        // Load preferred name if it exists
        QString preferredName = config["preferred_name"].toString();
        int nameIndex = userComboBox->findText(preferredName);
        if (nameIndex >= 0) {
            userComboBox->setCurrentIndex(nameIndex);
        }
        
        // Load audio device if it exists
        QString preferredDevice = config["audio_device"].toString();
        int deviceIndex = deviceComboBox->findText(preferredDevice);
        if (deviceIndex >= 0) {
            deviceComboBox->setCurrentIndex(deviceIndex);
        }
    }
}

void SettingsFrame::saveSettings() {
    QJsonObject config;
    
    // Save WebSocket settings
    config["ws_enabled"] = wsEnabledCheckBox->isChecked();
    config["ws_ip"] = wsIpEdit->text();
    config["ws_port"] = wsPortEdit->text();
    
    // Save push-to-talk settings
    config["push_to_talk_key"] = hotkeyEdit->text();
    config["recording_mode"] = recordingModeSwitch->isChecked() ? "toggle" : "push";
    
    // Save action hotkeys
    for (const auto &hotkey : actionHotkeys) {
        config[hotkey.name + "_hotkey"] = hotkey.edit->text();
    }
    
    // Save preferred name and device
    config["preferred_name"] = userComboBox->currentText();
    config["audio_device"] = deviceComboBox->currentText();
    
    QFile file("config.json");
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(config);
        file.write(doc.toJson(QJsonDocument::Indented));
        QMessageBox::information(this, "Settings", "Settings saved successfully!");
    } else {
        QMessageBox::warning(this, "Settings", "Failed to save settings!");
    }
}

void SettingsFrame::onWebSocketToggled(bool enabled) {
    wsIpEdit->setEnabled(enabled);
    wsPortEdit->setEnabled(enabled);
}

void SettingsFrame::onSetHotkeyClicked() {
    if (!isSettingHotkey) {
        isSettingHotkey = true;
        currentHotkeyTarget = "push_to_talk";
        hotkeyButton->setText("Press any key...");
        hotkeyEdit->clear();
        // TODO: Implement key capture
    }
}

void SettingsFrame::onRecordingModeChanged(bool toggleMode) {
    // TODO: Implement recording mode change
}

void SettingsFrame::onSetActionHotkeyClicked(const QString& action) {
    if (!isSettingHotkey) {
        isSettingHotkey = true;
        currentHotkeyTarget = action;
        
        for (auto &hotkey : actionHotkeys) {
            if (hotkey.name == action) {
                hotkey.button->setText("Press any key...");
                hotkey.edit->clear();
                break;
            }
        }
        // TODO: Implement key capture
    }
}

void SettingsFrame::onDeviceSelectionChanged(int index) {
    // TODO: Implement device change
}

void SettingsFrame::onUserSelectionChanged(int index) {
    // TODO: Implement user change
}

// Getters
QString SettingsFrame::getSelectedDevice() const {
    return deviceComboBox->currentText();
}

QString SettingsFrame::getSelectedUser() const {
    return userComboBox->currentText();
}

QString SettingsFrame::getWebSocketIP() const {
    return wsIpEdit->text();
}

QString SettingsFrame::getWebSocketPort() const {
    return wsPortEdit->text();
}

bool SettingsFrame::isWebSocketEnabled() const {
    return wsEnabledCheckBox->isChecked();
}

QString SettingsFrame::getPushToTalkKey() const {
    return hotkeyEdit->text();
}

bool SettingsFrame::isToggleModeEnabled() const {
    return recordingModeSwitch->isChecked();
}

} // namespace ui
} // namespace whisper_client