#include "clientsmanager.h"

ClientsManager::ClientsManager(QObject *parent) : QObject(parent)
{

}

void ClientsManager::clientsRecount()
{
	for( auto client:m_clients ){
		if( client->getUserLogin().isEmpty() ) continue;
		app::usersConnectionsNumAdd( client->getUserLogin(), 1 );
	}
}

void ClientsManager::incomingConnection(qintptr handle)
{
	Client* client = new Client();

	connect( client, &Client::signal_finished, this, &ClientsManager::slot_clientFinished );
	connect( client, &Client::signal_authOK, this, &ClientsManager::signal_recount );

	client->setSocketDescriptor( handle );
	m_clients.append( client );
}

void ClientsManager::slot_stop()
{
	for( auto client:m_clients ){
		client->stop();
		client->deleteLater();
	}
	m_clients.clear();
	emit signal_finished(this);
}

void ClientsManager::slot_start()
{

}

void ClientsManager::slot_clientFinished()
{
	for( auto client:m_clients ){
		if( client->isFinished() ){
			m_clients.removeOne(client);
			client->deleteLater();
			break;
		}
	}
}
