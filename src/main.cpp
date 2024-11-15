#include "ui/main_window.hpp"
#include <QApplication>
#include <stdexcept>
#include <iostream>

int main(int argc, char *argv[]) {
    try {
        QApplication app(argc, argv);

        // Set application style
        app.setStyle("Fusion");

        // Set dark theme palette
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
        darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);
        app.setPalette(darkPalette);

        whisper_client::ui::MainWindow mainWindow;
        mainWindow.start();

        return app.exec();
    } catch (const std::exception& e) {
        std::cerr << "Error starting application: " << e.what() << std::endl;
        return 1;
    }
}