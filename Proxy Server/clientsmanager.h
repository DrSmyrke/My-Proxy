#ifndef CLIENTSMANAGER_H
#define CLIENTSMANAGER_H

#include <QObject>
#include "client.h"

class ClientsManager : public QObject
{
	Q_OBJECT
public:
	explicit ClientsManager(QObject *parent = nullptr);
	uint16_t getClientsCount() { return m_clients.size(); }
	void incomingConnection(qintptr handle);
public slots:
	void slot_stop();
	void slot_start();
signals:
	void signal_finished();
private slots:
	void slot_clientFinished();
private:
	QList<Client*> m_clients;
};

#endif // CLIENTSMANAGER_H
