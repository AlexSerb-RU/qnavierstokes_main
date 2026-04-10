#include <QtWidgets/QApplication>
#include "main_window.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(true);

    MainWindow window;
    window.show();

    return app.exec();
}
