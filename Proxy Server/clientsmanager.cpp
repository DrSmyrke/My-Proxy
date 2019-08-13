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

void ClientsManager::slot_recount(){

	for( auto client:m_clients ){
		auto user = client->getUserLogin();
		if( !user.isEmpty() ) app::usersConnectionsNumAdd( user, 1 );
	}

	emit signal_recount( this, m_clients.size() );
}

void ClientsManager::slot_clientFinished()
{
	for( auto client:m_clients ){
		if( client->isFinished() ){
			auto user = client->getUserLogin();
			if( !user.isEmpty() ) app::usersConnectionsNumAdd( user, -1 );
			m_clients.removeOne(client);
			client->deleteLater();
			break;
		}
	}
}
