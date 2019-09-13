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
	uint32_t maxConnections = 37;
	QStringList accessList;
	QStringList blockList;
};

struct Config{
	bool verbose						= false;
	uint8_t logLevel					= 3;
#ifdef __linux__
	QString confFile					= "/etc/DrSmyrke/MyProxy/config.ini";
	QString logFile						= "/var/log/socksproxy.log";
	QString accessFile					= "/etc/DrSmyrke/MyProxy/access.list";
	QString usersFile					= "/etc/DrSmyrke/MyProxy/users.list";
#elif _WIN32
	QString confFile					= QDir::homePath() + "/MyProxy/config.ini";
	QString logFile						= QDir::homePath() + "/MyProxy/socksproxy.log";
	QString accessFile					= QDir::homePath() + "/MyProxy/access.list";
	QString usersFile					= QDir::homePath() + "/MyProxy/users.list";
#endif
	uint16_t port						= 7301;
	bool settingsSave					= false;
	bool usersSave						= false;
	std::vector<User> users;
	std::map<QString,uint32_t> usersConnections;
	QByteArray realmString				= "ProxyAuth";
	QString version;
};

struct BanData{
	QHostAddress addr;
	uint32_t sec;
};

struct AccessList{
	std::vector<QHostAddress> socks4Access;
	std::vector<QString> blackDomains;
	std::vector<QHostAddress> blackIPs;
	std::vector<QString> whiteDomains;
	std::vector<QHostAddress> whiteIPs;
	QList< BanData > banList;
	bool accessFileSave = false;
};

namespace app {
	extern Config conf;
	extern AccessList accessList;

	void loadSettings();
	void saveSettings();
	bool parsArgs(int argc, char *argv[]);
	void setLog(const uint8_t logLevel, const QString &mess);
	void addGlobalBlackAddr(const QString &str);
	void addGlobalBlackIP(const QHostAddress &addr);
	void updateBlackIPAddrs();
	bool isBlockAddr(const QHostAddress& addr);
	void loadAccessFile();
	void saveAccessFile();
	bool addUser(const QString &login, const QString &pass, const uint8_t group = UserGrpup::users);
	bool findUser(const QString &login);
	bool isSocks4Access(const QHostAddress &addr);
	bool chkAuth(const QString &login, const QString &pass);
	User getUserData(const QString &login);
	bool isBan(const QHostAddress &addr);
	void addBAN(const QHostAddress &addr, const uint8_t timeout = 30);
	bool isBlockedDomName(const QString &domName);
	void loadUsers();
	void saveUsers();
	bool isBlockedUserList(const QString &login, const QString &addr);
	bool isAccessUserList(const QString &login, const QString &addr);
	void changeUserConnection(const QString &login, const int connCount);
	uint32_t getUserConnectionsNum(const QString &login);
	uint8_t getUserGroupFromName(const QString &name);
	QString getUserGroupNameFromID(const uint8_t id);
	void updateUserLoginTimeStamp(const QString &login);
	void updateBanList();
}

#endif // GLOBAL_H
