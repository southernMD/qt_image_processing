#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    // char ARG_DISABLE_WEB_SECURITY[] = "--disable-web-security";
    // int newArgc = argc+1+1;
    // char** newArgv = new char*[newArgc];
    // for(int i=0; i<argc; i++) {
    //     newArgv[i] = argv[i];
    // }
    // newArgv[argc] = ARG_DISABLE_WEB_SECURITY;
    // newArgv[argc+1] = nullptr;

    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowTitle("大智慧图像处理V1.0 249400231徐哲轶");
    w.setWindowIcon(QIcon(":/favicon.ico"));
    w.show();
    return a.exec();
}
