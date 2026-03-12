#include <QApplication>

#include "app/DupeApp.h"
#include "types.h"
#include "ui/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("DupeCheck"));
    app.setOrganizationName(QStringLiteral("DupeCheck"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));

    // Register custom types for queued (cross-thread) signal/slot connections.
    qRegisterMetaType<QList<FileRecord>>("QList<FileRecord>");

    auto* dupeApp = new DupeApp;
    MainWindow window(dupeApp);
    window.show();

    return QApplication::exec();
}
