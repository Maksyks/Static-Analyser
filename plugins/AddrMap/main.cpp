#include "AddrMapPlugin.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AddrMapPlugin w;
    w.show();
    return a.exec();
}
