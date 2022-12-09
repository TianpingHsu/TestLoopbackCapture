#include "QtWidgetsApplication1.h"
#include <QtWidgets/QApplication>
#include <cstdio>

int main(int argc, char *argv[])
{
	(void)freopen("C:\\test.log", "a", stdout);
	(void)freopen("C:\\test.log", "a", stderr);
    QApplication a(argc, argv);
    QtWidgetsApplication1 w;
    w.show();
    return a.exec();
}
