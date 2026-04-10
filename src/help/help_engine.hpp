#pragma once
#include <QtCore/QtClassHelperMacros>
#include <QtCore/QScopedPointer>
#include <QtHelp/QHelpEngine>


class HelpEngine
{
public:
    static HelpEngine& instance()
    {
        static HelpEngine helpEngine;
        return helpEngine;
    }

    inline QHelpContentWidget* contentWidget(){
        return this->_helpEngine->contentWidget();
    }

    inline QHelpIndexWidget* indexWidget(){
        return this->_helpEngine->indexWidget();
    }

    inline QByteArray fileData(const QUrl &url) const {
        return this->_helpEngine->fileData(url);
    }

private:
    explicit HelpEngine();
    Q_DISABLE_COPY_MOVE(HelpEngine);

    QScopedPointer<QHelpEngine> _helpEngine;
};
