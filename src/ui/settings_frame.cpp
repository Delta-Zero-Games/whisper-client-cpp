#include "ui/settings_frame.hpp"
#include "ui/main_window.hpp"
#include "audio/audio_capture.hpp"
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QDebug>

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
    auto* deviceGroup = new QGroupBox("Audio Device", this);
    auto* deviceLayout = new QVBoxLayout(deviceGroup);

    // Create device combo box
    deviceComboBox = new QComboBox(this);
    deviceComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    deviceLayout->addWidget(deviceComboBox);

    // Add refresh button
    auto* refreshButton = new QPushButton("Refresh Devices", this);
    deviceLayout->addWidget(refreshButton);

    mainLayout->addWidget(deviceGroup);

    // Connect signals
    connect(deviceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsFrame::onDeviceSelectionChanged);
    connect(refreshButton, &QPushButton::clicked,
            this, &SettingsFrame::updateDeviceList);

    // Initial device list population
    updateDeviceList();
}

void SettingsFrame::createUserSection() {
    auto* userGroup = new QGroupBox("User", this);
    auto* userLayout = new QVBoxLayout(userGroup);
    
    userComboBox = new QComboBox(this);
    userLayout->addWidget(userComboBox);
    
    mainLayout->addWidget(userGroup);
    loadUserNames();
}

void SettingsFrame::createWebSocketSection() {
    auto* wsGroup = new QGroupBox("WebSocket", this);
    auto* wsLayout = new QVBoxLayout(wsGroup);
    
    // Enable checkbox
    wsEnabledCheckBox = new QCheckBox("Enable WebSocket", this);
    wsLayout->addWidget(wsEnabledCheckBox);
    
    // IP and Port fields
    auto* wsAddressLayout = new QHBoxLayout();
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
    auto* hotkeyGroup = new QGroupBox("Push to Talk", this);
    auto* hotkeyLayout = new QHBoxLayout(hotkeyGroup);
    
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
    auto* actionGroup = new QGroupBox("Twitch Action Hotkeys", this);
    auto* actionLayout = new QGridLayout(actionGroup);
    
    const QStringList actions = {"TTS", "Follows", "Subs", "Gifts"};
    
    for (int i = 0; i < actions.size(); ++i) {
        const QString &actionName = actions[i];
        
        auto* edit = new QLineEdit(this);
        edit->setReadOnly(true);
        
        auto* button = new QPushButton("Set " + actionName + " Key", this);
        
        actionLayout->addWidget(edit, i, 0);
        actionLayout->addWidget(button, i, 1);
        
        actionHotkeys.push_back({
            actionName.toLower(),
            edit,
            button,
            QString()
        });
        
        connect(button, &QPushButton::clicked, this, 
            [this, actionName](){ onSetActionHotkeyClicked(actionName.toLower()); });
    }
    
    mainLayout->addWidget(actionGroup);
}

void SettingsFrame::setupConnections() {
    connect(wsEnabledCheckBox, &QCheckBox::toggled, this, &SettingsFrame::onWebSocketToggled);
    connect(hotkeyButton, &QPushButton::clicked, this, &SettingsFrame::onSetHotkeyClicked);
    connect(recordingModeSwitch, &QCheckBox::toggled, this, &SettingsFrame::onRecordingModeChanged);
    connect(saveButton, &QPushButton::clicked, this, &SettingsFrame::saveSettings);
    connect(userComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsFrame::onUserSelectionChanged);
}

void SettingsFrame::updateDeviceList() {
    // Clear old devices
    deviceComboBox->clear();
    audioDevices.clear();

    try {
        // Get devices from AudioCapture
        whisper_client::audio::AudioCapture* audioCapture = 
            qobject_cast<MainWindow*>(window())->getAudioCapture();
        if (!audioCapture) return;

        std::vector<whisper_client::audio::AudioDevice> devices = audioCapture->listInputDevices();
        
        // Add devices to combo box
        for (const auto& device : devices) {
            deviceComboBox->addItem(device.name);
            audioDevices.push_back({device.id, device.name});
            
            // Mark default device
            if (device.isDefault) {
                deviceComboBox->setItemText(deviceComboBox->count() - 1,
                                          device.name + " (Default)");
            }
        }

        // Set to saved device if available
        QString savedDevice = config.value("audio_device").toString();
        if (!savedDevice.isEmpty()) {
            int index = deviceComboBox->findText(savedDevice);
            if (index >= 0) {
                deviceComboBox->setCurrentIndex(index);
            }
        }
    }
    catch (const std::exception& e) {
        qWarning() << "Error updating device list:" << e.what();
    }
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
        QJsonObject jsonConfig = doc.object();
        
        // Convert JSON config to QVariant map
        for (auto it = jsonConfig.begin(); it != jsonConfig.end(); ++it) {
            config[it.key()] = it.value().toVariant();
        }
        
        // Load WebSocket settings
        wsEnabledCheckBox->setChecked(config.value("ws_enabled", true).toBool());
        wsIpEdit->setText(config.value("ws_ip", "localhost").toString());
        wsPortEdit->setText(config.value("ws_port", "3001").toString());
        
        // Load push-to-talk settings
        hotkeyEdit->setText(config.value("push_to_talk_key", "f5").toString());
        recordingModeSwitch->setChecked(config.value("recording_mode", "push").toString() == "toggle");
        
        // Load action hotkeys
        for (auto &hotkey : actionHotkeys) {
            QString key = hotkey.name + "_hotkey";
            QString value = config.value(key).toString();
            hotkey.key = value;
            hotkey.edit->setText(value);
        }
        
        // Load preferred name if it exists
        QString preferredName = config.value("preferred_name").toString();
        int nameIndex = userComboBox->findText(preferredName);
        if (nameIndex >= 0) {
            userComboBox->setCurrentIndex(nameIndex);
        }

        // Update device list and selection
        updateDeviceList();
    }
}

void SettingsFrame::saveSettings() {
    // Update config map
    config["ws_enabled"] = wsEnabledCheckBox->isChecked();
    config["ws_ip"] = wsIpEdit->text();
    config["ws_port"] = wsPortEdit->text();
    config["push_to_talk_key"] = hotkeyEdit->text();
    config["recording_mode"] = recordingModeSwitch->isChecked() ? "toggle" : "push";
    config["preferred_name"] = userComboBox->currentText();
    config["audio_device"] = deviceComboBox->currentText();
    
    // Save action hotkeys
    for (const auto &hotkey : actionHotkeys) {
        config[hotkey.name + "_hotkey"] = hotkey.edit->text();
    }
    
    // Save to file
    QJsonObject jsonConfig;
    for (auto it = config.begin(); it != config.end(); ++it) {
        jsonConfig[it.key()] = QJsonValue::fromVariant(it.value());
    }
    
    QFile file("config.json");
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(jsonConfig);
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
    }
}

void SettingsFrame::onRecordingModeChanged(bool toggleMode) {
    config["recording_mode"] = toggleMode ? "toggle" : "push";
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
    }
}

void SettingsFrame::onDeviceSelectionChanged(int index) {
    if (index < 0 || index >= static_cast<int>(audioDevices.size())) return;

    whisper_client::audio::AudioCapture* audioCapture = 
        qobject_cast<MainWindow*>(window())->getAudioCapture();
    if (!audioCapture) return;

    const auto& device = audioDevices[index];
    if (audioCapture->setDevice(device.id)) {
        config["audio_device"] = device.name;
        qDebug() << "Audio device set to:" << device.name;
    }
}

void SettingsFrame::onUserSelectionChanged(int index) {
    if (index >= 0) {
        config["preferred_name"] = userComboBox->currentText();
    }
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

QString SettingsFrame::getActionHotkey(const QString& action) const {
    for (const auto& hotkey : actionHotkeys) {
        if (hotkey.name == action) {
            return hotkey.key;
        }
    }
    return QString();
}

} // namespace ui
} // namespace whisper_client