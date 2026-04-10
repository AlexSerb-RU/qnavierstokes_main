#include <QtCore/QDir>
#include <QtWidgets/QApplication>
#include "help_engine.hpp"


HelpEngine::HelpEngine():
    _helpEngine(new QHelpEngine(QDir::cleanPath(qApp->applicationDirPath() + QDir::separator() + "qnavierstokes.qch")))
{
    this->_helpEngine->setupData();
}
