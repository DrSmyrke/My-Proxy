#include "server.h"

Server::Server(QObject *parent)	: QTcpServer(parent)
{
	app::setLog( 0, "SERVER CREATING..." );

	m_pThreadManager = new ThreadManager(this);
	m_pTimer = new QTimer(this);
		m_pTimer->setInterval(60000);

	connect( m_pTimer, &QTimer::timeout, this, [this](){
		m_pThreadManager->clientsRecount();
		app::saveDataToBase();
		if( app::state.urls.size() > 0 ){
			while( app::state.urls.size() > app::conf.maxStatusListSize ) app::state.urls.erase( app::state.urls.begin() );
		}
		if( app::state.addrs.size() > 0 ){
			while( app::state.addrs.size() > app::conf.maxStatusListSize ) app::state.addrs.erase( app::state.addrs.begin() );
		}
	} );
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
	if( app::sdb.isOpen() ) app::sdb.close();
	app::saveSettings();
	this->close();
}

void Server::incomingConnection(qintptr handle)
{
	m_pThreadManager->incomingConnection(handle);
}
