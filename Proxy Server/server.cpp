#include "server.h"

Server::Server(QObject *parent)	: QTcpServer(parent)
{
	app::setLog( 0, "SERVER CREATING..." );

	m_pThreadManager = new ThreadManager(this);
	m_pTimer = new QTimer(this);
		m_pTimer->setInterval(100);

	connect( m_pTimer, &QTimer::timeout, this, [this](){
		uint8_t tickPerSecond = 1000 / m_pTimer->interval();
		if( m_timerCounter++ % tickPerSecond ){
			if( m_secSaveSettingsCounter == 3 ){
				app::saveSettings();
				m_secSaveSettingsCounter = 1;
			}
			if( m_secUpdateDataCounter == 60 ){
				m_pThreadManager->clientsRecount();
				if( app::state.urls.size() > 0 ){
					while( app::state.urls.size() > app::conf.maxStatusListSize ) app::state.urls.erase( app::state.urls.begin() );
				}
				if( app::state.addrs.size() > 0 ){
					while( app::state.addrs.size() > app::conf.maxStatusListSize ) app::state.addrs.erase( app::state.addrs.begin() );
				}
				m_secUpdateDataCounter = 1;
			}

			m_secSaveSettingsCounter++;
			m_secUpdateDataCounter++;
		}
	} );
}

void Server::run()
{
	if(!this->listen(QHostAddress::AnyIPv4,app::conf.port)){
		qWarning("ERROR\n");
	}else{
		m_pTimer->start();
		app::setLog( 0, QString("SERVER [ ACTIVATED ] PORT: [%1]").arg(app::conf.port) );
	}
}

void Server::stop()
{
	app::setLog( 0, "SERVER STOPPING..." );
	m_pThreadManager->stop();
	if( m_pTimer->isActive() ) m_pTimer->stop();
	app::saveSettings();
	this->close();
}

void Server::incomingConnection(qintptr handle)
{
	m_pThreadManager->incomingConnection(handle);
}
