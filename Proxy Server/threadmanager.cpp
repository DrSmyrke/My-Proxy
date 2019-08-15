#include "threadmanager.h"
#include <QThread>

ThreadManager::ThreadManager(QObject *parent) : QObject(parent)
{

}

void ThreadManager::incomingConnection(qintptr handle)
{
	if( app::conf.maxThreads == 0 ){
		disconnectClient(handle);
		return;
	}

	bool freeSpaceF = false;
	for( auto &clientManager:m_clientsManager ){
		if( clientManager->getClientsCount() < app::conf.maxClients ){
			clientManager->incomingConnection(handle);
			freeSpaceF = true;
			break;
		}
	}

	if( !freeSpaceF ){
		if( m_clientsManager.size() < app::conf.maxThreads ){
			newThread(handle);
		}else{
			disconnectClient(handle);
		}
	}

	for( auto clientManager:m_clientsManager ){
		if( clientManager->getClientsCount() == 0 ){
			clientManager->slot_stop();
			m_clientsManager.removeOne( clientManager );
			break;
		}
	}

	clientsRecount();
}

void ThreadManager::stop()
{
	emit signal_stopAll();
}

void ThreadManager::disconnectClient(qintptr handle)
{
	QTcpSocket* client = new QTcpSocket(this);
	client->setSocketDescriptor(handle);
	client->close();
	client->deleteLater();
}

void ThreadManager::newThread(qintptr handle)
{
	ClientsManager* manager = new ClientsManager();
	QThread* thread = new QThread();
	manager->moveToThread(thread);

	connect( thread, &QThread::started, manager, &ClientsManager::slot_start );
	connect( manager, &ClientsManager::signal_finished ,thread, &QThread::quit );
	connect( this, &ThreadManager::signal_stopAll, manager, &ClientsManager::slot_stop );
	connect( this, &ThreadManager::signal_recount, manager, &ClientsManager::slot_recount );

	connect( manager, &ClientsManager::signal_finished, this, [this](ClientsManager* cmanager){
		m_clientsManager.removeOne(cmanager);
		cmanager->deleteLater();
		clientsRecount();
	} );
	connect( thread, &QThread::finished, thread, &QThread::deleteLater );

	thread->start();
	manager->incomingConnection( handle );

	m_clientsManager.append( manager );
}

void ThreadManager::clientsRecount()
{
	app::state.threads.clear();
	app::usersConnectionsClear();

	emit signal_recount();
}
