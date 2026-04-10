#include <QtWidgets/QFileDialog>
#include "settings_window.hpp"
#include "ui_settings_window.h"
#include "settings_manager.hpp"

SettingsWindow::SettingsWindow(QWidget *parent) : QDialog(parent), _ui(new Ui::SettingsWindow)
{
    this->_ui->setupUi(this);
    this->_adaptSettings();

    this->connect(
        this->_ui->cancelButton, &QPushButton::clicked,
        this, &SettingsWindow::_rollbackSettings
    );
    this->connect(
        this->_ui->okButton, &QPushButton::clicked,
        this, &SettingsWindow::_commitCommitSettings
    );
}

SettingsWindow::~SettingsWindow()
{
    delete this->_ui;
}

void SettingsWindow::_adaptSettings()
{
    auto settings = SettingsManager::instance().settings();

    this->_ui->useUniqFoldersCheckBox->setChecked(settings.useUniqueFolders);
    this->_ui->restrictPrGr->setChecked(settings.limitPrGr);
    this->_ui->useScientificNotation->setChecked(settings.scientificPrGr);

    switch(settings.paintEngine)
    {
    case PaintEngine::OpenGL:
        this->_ui->paintEngineComboBox->setCurrentIndex(0);
        break;
    }

    switch(settings.colorScheme)
    {
    case ColorScheme::BlueRed:
        this->_ui->glColorSchemeComboBox->setCurrentIndex(0);
        break;
    case ColorScheme::Rainbow:
        this->_ui->glColorSchemeComboBox->setCurrentIndex(1);
        break;
    }
}

void SettingsWindow::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type())
    {
    case QEvent::LanguageChange:
        this->_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void SettingsWindow::_commitCommitSettings()
{
    Settings settings;

    settings.useUniqueFolders = this->_ui->useUniqFoldersCheckBox->isChecked();
    settings.limitPrGr = this->_ui->restrictPrGr->isChecked();
    settings.scientificPrGr = this->_ui->useScientificNotation->isChecked();

    if(this->_ui->paintEngineComboBox->currentIndex() == 0){
        settings.paintEngine = PaintEngine::OpenGL;
    }

    if(this->_ui->glColorSchemeComboBox->currentIndex() == 0){
        settings.colorScheme = ColorScheme::BlueRed;
    }

    SettingsManager::instance().setSettings(settings);

    this->close();
}

void SettingsWindow::_rollbackSettings()
{
    this->_adaptSettings();
    this->close();
}
