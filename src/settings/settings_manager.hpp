#pragma once
#include <QtCore/QObject>
#include "settings.hpp"


class SettingsManager : public QObject
{
    Q_OBJECT

public:
    static SettingsManager& instance()
    {
        static SettingsManager manager;
        return manager;
    }

    inline const Settings& settings()
    {
        return this->_settings;
    }

    inline void setSettings(Settings settings)
    {
        this->_settings = settings;
        this->_saveSettings();
        emit this->settingsChanged(this->_settings);
    }

signals:
    void settingsChanged(const Settings&);

private:
    explicit SettingsManager(QObject *parent = nullptr);
    Q_DISABLE_COPY_MOVE(SettingsManager);

    static QString _settingsPath();
    void _loadSettings();
    void _saveSettings() const;

    Settings _settings;
};
