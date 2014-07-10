#include <QApplication>
#include <QCleanlooksStyle>
#include "mainwindow.h"

QString app_dir;
unsigned char color_spectrum_value(unsigned char center, unsigned char value)
{
    unsigned char dif = center > value ? center-value:value-center;
    if(dif < 32)
        return 255;
    dif -= 32;
    if(dif >= 64)
        return 0;
    return 255-(dif << 2);
}
image::color_image bar,colormap;

int main(int argc, char *argv[])
{
    {
        colormap.resize(image::geometry<2>(256,1));
        bar.resize(image::geometry<2>(20,256));
        for(unsigned int index = 0;index < 256;++index)
        {
            image::rgb_color color;
            color.r = color_spectrum_value(64,index);
            color.g = color_spectrum_value(128,index);
            color.b = color_spectrum_value(128+64,index);
            colormap[255-index] = color;
            if(index && index != 255)
            {
                int sep = (index % 51 == 0) ? 5 : 1;
                std::fill(bar.begin()+index*20+sep,bar.begin()+(index+1)*20-sep,color);
            }
        }
    }

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
