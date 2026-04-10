#pragma once
#include <QtWidgets/QDialog>

namespace Ui
{
    class SettingsWindow;
}

class SettingsWindow : public QDialog
{
    Q_OBJECT

public:
    SettingsWindow(QWidget *parent = 0);

    ~SettingsWindow();

protected:
    void changeEvent(QEvent *e);

private:
    void _adaptSettings();

    Ui::SettingsWindow *_ui;

private slots:
    void _rollbackSettings();

    void _commitCommitSettings();
};
