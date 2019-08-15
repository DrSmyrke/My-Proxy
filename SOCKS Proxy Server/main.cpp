#include <QCoreApplication>
#include "server.h"
#include "global.h"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	app::loadSettings();
	if( !app::parsArgs(argc, argv) ) return 0;

	app::updateBlackIPAddrs();

	if( !app::conf.logFile.isEmpty() ) QFile( app::conf.logFile ).remove();

	//TODO:remove
	//app::conf.socksClients.push_back( "192.168.1.250" );
	//app::conf.socksClients.push_back( "127.0.0.1" );
	//app::addUser( "admin", "admin", UserGrpup::admins );
	//app::addUser( "test", "test" );

	//Server* server = new Server();
	//server->run();
	Server server;
	if( !server.run() ) return 0;

	return a.exec();
}

