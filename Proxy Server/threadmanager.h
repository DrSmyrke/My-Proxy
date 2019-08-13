#ifndef THREADMANAGER_H
#define THREADMANAGER_H

#include <QObject>
#include <QTcpSocket>
#include "clientsmanager.h"
#include "global.h"

class ThreadManager : public QObject
{
	Q_OBJECT
public:
	explicit ThreadManager(QObject *parent = nullptr);
	void incomingConnection(qintptr handle);
	void stop();
	void clientsRecount();
signals:
	void signal_stopAll();
	void signal_recount();
private:
	QList<ClientsManager*> m_clientsManager;

	void disconnectClient(qintptr handle);
	void newThread(qintptr handle);
};

#endif // THREADMANAGER_H
