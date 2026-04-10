#pragma once
#include <QTextBrowser>

class HelpBrowser : public QTextBrowser
{
public:
    explicit HelpBrowser(QWidget* parent = nullptr);
    QVariant loadResource (int type, const QUrl& name);
};
