#include <QCoreApplication>
#include "server.h"
#include "global.h"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	app::loadSettings();
	if( !app::parsArgs(argc, argv) ) return 0;

	//TODO:remove
	app::conf.socksClients.push_back( "192.168.1.250" );
	app::conf.socksClients.push_back( "127.0.0.1" );
	app::addUser( "admin", "admin", UserGrpup::admins );

	Server* server=new Server();
	server->run();

	return a.exec();
}

