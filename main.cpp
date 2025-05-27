#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowTitle("大智慧图像处理V1.0 249400231徐哲轶");
    w.setWindowIcon(QIcon(":/favicon.ico"));
    w.show();
    return a.exec();
}
