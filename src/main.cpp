#include "MainWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Nicolas Truffier"));
    QCoreApplication::setApplicationName(QStringLiteral("Mach3SurfacingGenerator"));
    QCoreApplication::setApplicationVersion(QStringLiteral(MACH3_APP_VERSION));
    QApplication::setApplicationDisplayName(QStringLiteral("Mach3 Surfacing Generator"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/branding/logo.png")));

    MainWindow window;
    window.show();

    return application.exec();
}
