#include <QtCore/QDir>
#include <QtWidgets/QApplication>
#include <QtHelp/QHelpContentWidget>
#include <QtHelp/QHelpIndexWidget>
#include <QtHelp/QHelpLink>
#include "ui_help_window.h"
#include "help_window.hpp"
#include "help_engine.hpp"
#include "help_browser.hpp"

HelpWindow::HelpWindow(QWidget *parent): QMainWindow(parent),
    _ui(new Ui::HelpWindow)
{
    this->_ui->setupUi(this);

    HelpEngine& helpEngine = HelpEngine::instance();
    QHelpContentWidget* contentWidget = helpEngine.contentWidget();
    QHelpIndexWidget* indexWidget = helpEngine.indexWidget();

    this->_ui->tabWidget->addTab(contentWidget, "Содержание");
    this->_ui->tabWidget->addTab(indexWidget, "Ключевые слова");

    HelpBrowser* helpBrowser = new HelpBrowser(this);
    helpBrowser->setSource(QUrl("qthelp://amilab/doc/content/index.htm"));

    this->_ui->horizontalLayout->addWidget(helpBrowser);

    this->connect(
        contentWidget, &QHelpContentWidget::linkActivated,
        helpBrowser, [=](const QUrl &link){helpBrowser->setSource(link);}
    );

    this->connect(
        indexWidget, &QHelpIndexWidget::documentActivated,
        helpBrowser, [=](const QHelpLink &document, const QString &keyword){helpBrowser->setSource(document.url);}
    );
}
