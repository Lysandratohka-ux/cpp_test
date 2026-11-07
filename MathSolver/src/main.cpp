#include "window.h"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QStandardPaths>
#include <QTimer>
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("MathSolverCourse");
    QCoreApplication::setApplicationName("MathSolver");
    QCoreApplication::setApplicationVersion("2.4.1");
    if (app.arguments().contains("--smoke"))
        QStandardPaths::setTestModeEnabled(true);
    auto contents = QCoreApplication::applicationDirPath() + "/..";
    auto helper = contents + "/Helpers/QtWebEngineProcess";
    if (QFileInfo::exists(helper)) {
        qputenv("QTWEBENGINEPROCESS_PATH", QDir::cleanPath(helper).toUtf8());
        qputenv("QTWEBENGINE_RESOURCES_PATH",
                QDir::cleanPath(contents + "/Resources/qtwebengine").toUtf8());
        qputenv("QTWEBENGINE_LOCALES_PATH",
                QDir::cleanPath(contents + "/Resources/qtwebengine_locales").toUtf8());
    }
    app.setApplicationDisplayName("Infinity");
    app.setWindowIcon(QIcon(":/icons/infinity.png"));
    auto definitions = QCoreApplication::applicationDirPath() + "/../Resources/qalculate";
    if (!QDir(definitions).exists())
        definitions = QCoreApplication::applicationDirPath() + "/resources/qalculate";
    if (QDir(definitions).exists())
        qputenv("QALCULATE_DEFINITIONS_DIR", QDir(definitions).absolutePath().toUtf8());
    Window w;
    w.show();
    if (app.arguments().contains("--smoke")) {
        QTimer::singleShot(2000, &app, [&] {
            w.selfTest([&](bool ok) {
                // Leave the WebEngine JavaScript callback before grabbing the
                // window or shutting down its pages; nested rendering can deadlock.
                QTimer::singleShot(0, &app, [&, ok] {
                    w.grab().save("gui-smoke.png");
                    app.exit(ok ? 0 : 2);
                });
            });
        });
        QTimer::singleShot(95000, &app, [&] { app.exit(3); });
    }
    return app.exec();
}
