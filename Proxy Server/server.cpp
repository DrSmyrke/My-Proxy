#include "server.h"

Server::Server(QObject *parent)	: QTcpServer(parent)
{
	app::setLog( 0, "SERVER CREATING..." );

	m_pThreadManager = new ThreadManager(this);
}

void Server::run()
{
	if(!this->listen(QHostAddress::AnyIPv4,app::conf.port)){
		qWarning("ERROR\n");
	}else{
		app::setLog( 0, QString("SERVER [ ACTIVATED ] PORT: [%1]").arg(app::conf.port) );
	}
}

void Server::stop()
{
	app::setLog( 0, "SERVER STOPPING..." );
	m_pThreadManager->stop();
	this->close();
}

void Server::incomingConnection(qintptr handle)
{
	m_pThreadManager->incomingConnection(handle);
}
