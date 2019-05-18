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
	connect( manager, &ClientsManager::signal_finished, manager, &ClientsManager::deleteLater );
	connect( thread, &QThread::finished, thread, &QThread::deleteLater );

	thread->start();
	manager->incomingConnection( handle );
}
