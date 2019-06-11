#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QFile>
#include <QTimer>
#include "client.h"
#include "threadmanager.h"
#include "global.h"

class Server : public QTcpServer
{
	Q_OBJECT
public:
	explicit Server(QObject *parent = 0);
	void run();
	void stop();
protected:
	void incomingConnection(qintptr handle);
private:
	ThreadManager* m_pThreadManager;
	QTimer* m_pTimer;
};

#endif // SERVER_H
