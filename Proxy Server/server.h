#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QFile>
#include <QTimer>
//#include "client.h"
#include "threadmanager.h"
#include "global.h"

class Server : public QTcpServer
{
	Q_OBJECT
public:
	explicit Server(QObject *parent = 0);
	bool run();
	void stop();
protected:
	void incomingConnection(qintptr handle);
private:
	ThreadManager* m_pThreadManager;
	QTimer* m_pTimer;
	uint8_t m_timerCounter = 0;
	uint8_t m_secSaveSettingsCounter = 0;
	uint8_t m_secUpdateDataCounter = 0;
};

#endif // SERVER_H
