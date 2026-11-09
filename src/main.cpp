#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QLoggingCategory>
#include <QIcon>

int main(int argc,char**argv){QApplication app(argc,argv);QApplication::setApplicationName("LOQ RGB Studio");QApplication::setOrganizationName("LOQ RGB Studio");QApplication::setApplicationVersion(LOQ_APP_VERSION);app.setDesktopFileName("loq-rgb-studio.desktop");QApplication::setQuitOnLastWindowClosed(false);app.setWindowIcon(QIcon::fromTheme("loq-rgb-studio"));QLoggingCategory::setFilterRules("*.debug=false\nqt.qpa.*=false");bool restore=app.arguments().contains("--restore");MainWindow window(restore);if(!restore)window.show();return app.exec();}
