#include <QtCore/QSettings>
#include <QtCore/QDir>
#include <QtWidgets/QApplication>
#include "settings_manager.hpp"
#include "settings.hpp"

SettingsManager::SettingsManager(QObject *parent) : QObject{parent}
{
    this->_loadSettings();
}

QString SettingsManager::_settingsPath()
{
    return QDir::cleanPath(qApp->applicationDirPath() + QDir::separator() + "qnavierstokes.ini");
}

void SettingsManager::_loadSettings()
{
    QSettings settings(SettingsManager::_settingsPath(), QSettings::IniFormat);
    this->_settings.useUniqueFolders = settings.value("common/use_unique_folders", QVariant(false)).toBool();
    this->_settings.limitPrGr = settings.value("common/limit_pr_gr", QVariant(false)).toBool();
    this->_settings.scientificPrGr = settings.value("experimental/scientific_pr_gr", QVariant(false)).toBool();

    if (settings.contains("painting/engine")) {
        this->_settings.paintEngine = static_cast<PaintEngine>(settings.value("painting/engine").toInt());
    } else {
        this->_settings.paintEngine = PaintEngine::OpenGL;
    }

    if (settings.contains("painting/color_scheme")) {
        this->_settings.colorScheme = static_cast<ColorScheme>(settings.value("painting/color_scheme").toInt());
    } else {
        this->_settings.colorScheme = ColorScheme::BlueRed;
    }
}

void SettingsManager::_saveSettings() const
{
    QSettings settings(SettingsManager::_settingsPath(), QSettings::IniFormat);
    settings.setValue("common/use_unique_folders", QVariant(this->_settings.useUniqueFolders));
    settings.setValue("painting/engine", QVariant(static_cast<int>(this->_settings.paintEngine)));
    settings.setValue("painting/color_scheme", QVariant(static_cast<int>(this->_settings.colorScheme)));
    settings.setValue("common/limit_pr_gr", QVariant(this->_settings.limitPrGr));
    settings.setValue("experimental/scientific_pr_gr", QVariant(this->_settings.scientificPrGr));
}
