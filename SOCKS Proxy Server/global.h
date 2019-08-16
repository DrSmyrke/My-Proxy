#ifndef GLOBAL_H
#define GLOBAL_H

#include <QString>
#include <QDir>
#include <QHostAddress>
#include <vector>

struct UserGrpup{
	enum{
		users,
		admins,
	};
};

struct User{
	QString login;
	QString pass;
	uint8_t group = UserGrpup::users;
	uint32_t lastLoginTimestamp = 0;
	uint32_t connections = 0;
	uint32_t maxConnections = 37;
};

struct Config{
	bool verbose						= false;
	uint8_t logLevel					= 3;
#ifdef __linux__
	QString logFile						= "/var/log/socksproxy.log";
	QString blackAddrsFile				= "/usr/share/MyProxy/blackAddrs.list";
	QString socks4AccessFile			= "/usr/share/MyProxy/socks4Access.list";
#elif _WIN32
	QString logFile						= QDir::homePath() + "/MyProxy/socksproxy.log";
	QString blackAddrsFile				= QDir::homePath() + "/MyProxy/blackAddrs.list";
	QString socks4AccessFile			= QDir::homePath() + "/MyProxy/socks4Access.list";
#endif
	uint16_t port						= 7301;
	bool saveSettings					= false;
	std::vector<User> users;
	QByteArray realmString				= "ProxyAuth";
};

struct BlackList{
	std::vector<QString> nameAddrs;
	std::vector<QHostAddress> ipAddrs;
	bool addrsFileSave = false;
};

struct AccessLists{
	std::vector<QHostAddress> socks4access;
	bool socks4AccessFileFileSave = false;
};

namespace app {
	extern Config conf;
	extern BlackList blackList;
	extern AccessLists accessLists;

	void loadSettings();
	void saveSettings();
	bool parsArgs(int argc, char *argv[]);
	void setLog(const uint8_t logLevel, const QString &mess);
	void loadBlackList(const QString &fileName, std::vector<QString> &data);
	void saveBlackList(const QString &fileName, const std::vector<QString> &data);
	void addGlobalBlackAddr(const QString &str);
	void updateBlackIPAddrs();
	bool findBlockAddr(const QHostAddress& addr);
	void loadAccessFile(const QString &fileName, std::vector<QHostAddress> &data);
	void saveAccessFile(const QString &fileName, std::vector<QHostAddress> &data);
	bool addUser(const QString &login, const QString &pass, const uint8_t group = UserGrpup::users);
	bool findUser(const QString &login);
	bool isSocks4Access(const QHostAddress &addr);
}

#endif // GLOBAL_H
