#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QtNetwork/QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include "client.h"
#include "global.h"

class Server : public QTcpServer
{
	Q_OBJECT
public:
	explicit Server(QObject *parent = 0);
	~Server();
	bool run();
	void stop();
protected:
	void incomingConnection(qintptr socketDescriptor);
signals:
	void signal_stopAll();
private slots:
	void slot_timer();
private:
	QTimer* m_pTimer;
};

#endif // SERVER_H
