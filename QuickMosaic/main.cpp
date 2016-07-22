#include "quickmosaic.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QuickMosaic w;
	w.show();
	return a.exec();
}
