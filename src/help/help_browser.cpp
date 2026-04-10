#include "help_browser.hpp"
#include "help_engine.hpp"

HelpBrowser::HelpBrowser(QWidget* parent): QTextBrowser(parent)
{
}

QVariant HelpBrowser::loadResource(int type, const QUrl &name){
    if (name.scheme() == "qthelp") {
        HelpEngine& helpEngine = HelpEngine::instance();
        return QVariant(helpEngine.fileData(name));
    }
    else {
        return QTextBrowser::loadResource(type, name);
    }
}
