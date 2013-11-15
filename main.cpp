#include <QApplication>
#include <QCleanlooksStyle>
#include "mainwindow.h"

QString app_dir;

int main(int argc, char *argv[])
{
    app_dir = argv[0];
    QApplication::setStyle(new QCleanlooksStyle);
    QApplication a(argc, argv);
    a.setOrganizationName("LabSolver");
    a.setApplicationName("ImageA");
    QFont font;
    font.setFamily(QString::fromUtf8("Arial"));
    a.setFont(font);
    MainWindow w;
    w.setFont(font);
    w.showMaximized();
    return a.exec();
}
