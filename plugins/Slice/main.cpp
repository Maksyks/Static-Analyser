#include "SlicePlugin.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SlicePlugin w;
    w.show();
    return a.exec();
}
