#include <QCoreApplication>
#include "server.h"
#include "controlserver.h"
#include "global.h"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	app::conf.version = QString("%1.%2").arg(APP_VER_FIRST).arg(APP_VER_SECOND);

	app::loadSettings();
	if( !app::parsArgs(argc, argv) ) return 0;

	if( !app::conf.logFile.isEmpty() ) QFile( app::conf.logFile ).remove();

	Server server;
	if( !server.run() ) return 0;

	ControlServer controlServer;
	if( !controlServer.run() ) return 0;

	return a.exec();
}

