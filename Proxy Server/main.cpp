#include <QCoreApplication>
#include "server.h"
#include "global.h"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	app::loadSettings();
	if( !app::parsArgs(argc, argv) ) return 0;

	Server* server=new Server();
	server->run();

	return a.exec();
}

