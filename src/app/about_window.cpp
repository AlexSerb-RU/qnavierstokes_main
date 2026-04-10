#include "about_window.hpp"
#include "ui_about_window.h"

AboutWindow::AboutWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutWindow)
{
    ui->setupUi(this);
}

AboutWindow::~AboutWindow()
{
    delete ui;
}

void AboutWindow::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
