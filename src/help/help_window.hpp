#pragma once
#include <QtCore/QScopedPointer>
#include <QtWidgets/QMainWindow>

namespace Ui
{
    class HelpWindow;
}


class HelpWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit HelpWindow(QWidget *parent = nullptr);

private:
    Ui::HelpWindow *_ui;
};
