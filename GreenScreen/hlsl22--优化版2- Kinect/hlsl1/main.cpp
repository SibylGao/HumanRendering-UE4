#include "hlsl1.h"
#include <QtWidgets/QApplication>
#include"QtGuiClass.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	hlsl1 w;
	w.show();
	
	/*QtGuiClass m("F:\\20191219.jpg");
	m.show();*/
	return a.exec();
}
