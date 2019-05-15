#include "visualizer.h"
#include <QApplication>

#include <QTimer>
#include <QObject>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Visualizer w;
    w.show();

    return a.exec();
}
