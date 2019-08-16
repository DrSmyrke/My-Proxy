#include "server.h"

Server::Server(QObject *parent)	: QTcpServer(parent)
{
	app::setLog( 0, "SERVER CREATING..." );
}

Server::~Server()
{
	app::setLog( 0, "SERVER DIE..." );
	emit signal_stopAll();
}

bool Server::run()
{
	if(!this->listen(QHostAddress::AnyIPv4,app::conf.port)){
		app::setLog( 0, QString("SERVER [ NOT ACTIVATED ] PORT: [%1] %2").arg(app::conf.port).arg(this->errorString()) );
		return false;
	}

	app::setLog( 0, QString("SERVER [ ACTIVATED ] PORT: [%1]").arg(app::conf.port) );

	return true;
}

void Server::stop()
{
	app::setLog( 0, "SERVER STOPPING..." );
	app::saveSettings();
	this->close();
}

void Server::incomingConnection(qintptr socketDescriptor)
{
	Client* client = new Client(socketDescriptor);
	//QThread* thread = new QThread();
	//client->moveToThread(thread);
//	connect(thread,&QThread::started,client,&Client::slot_start);
//	connect(client,&Client::signal_finished,thread,&QThread::quit);
	connect(this,&Server::signal_stopAll,client,&Client::slot_stop);
	connect(client,&Client::signal_finished,client,&Client::deleteLater);
//	connect(thread,&QThread::finished,thread,&QThread::deleteLater);
//	thread->start();
	client->run();
}