#ifndef CLIENTSMANAGER_H
#define CLIENTSMANAGER_H

#include <QObject>
#include "client.h"

class ClientsManager : public QObject
{
	Q_OBJECT
public:
	explicit ClientsManager(QObject *parent = nullptr);
	uint8_t getClientsCount() { return static_cast<uint8_t>(m_clients.size()); }
	void clientsRecount();
	void incomingConnection(qintptr handle);
public slots:
	void slot_stop();
	void slot_start();
	void slot_recount();
signals:
	void signal_finished(ClientsManager* manager);
private slots:
	void slot_clientFinished();
private:
	QList<Client*> m_clients;
};

#endif // CLIENTSMANAGER_H
