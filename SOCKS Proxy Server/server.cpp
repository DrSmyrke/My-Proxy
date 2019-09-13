#include "server.h"

Server::Server(QObject *parent)	: QTcpServer(parent)
{
	app::setLog( 0, QString("SERVER CREATING v%1 ...").arg(app::conf.version) );

	m_pTimer = new QTimer(this);
		m_pTimer->setInterval( 1000 );

	connect( m_pTimer, &QTimer::timeout, this, &Server::slot_timer );
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
	m_pTimer->start();

	return true;
}

void Server::stop()
{
	app::setLog( 0, "SERVER STOPPING..." );
	if( m_pTimer->isActive() ) m_pTimer->stop();
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

void Server::slot_timer()
{
	//проверка бан листа
	app::updateBanList();

	// сохранение настроек
	app::saveSettings();
}
