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

struct Control{
	enum{
		VERSION				= 0x01,
		USERS				= 0x02,
		BLACKLIST_ADDRS		= 0x03,
		AUTH				= 0x37,
		INFO				= 0x73,
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
	QStringList currentConnections;
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
	uint16_t controlPort				= 7302;
	bool settingsSave					= false;
	bool usersSave						= false;
	std::vector<User> users;
	std::map<QString,uint32_t> usersConnections;
	QByteArray realmString				= "ProxyAuth";
	QString version;
};

struct BanData{
	QHostAddress addr;
	uint8_t sec;
};

struct AccessList{
	std::vector<QHostAddress> socks4Access;
	std::vector<QString> blackDomains;
	std::vector<QHostAddress> blackIPs;
	std::vector<QHostAddress> blackIPsDynamic;
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
	void addGlobalBlackIPDynamic(const QHostAddress &addr);
	void updateBlackIPAddrs();
	bool isBlockAddr(const QHostAddress& addr);
	void loadAccessFile();
	void saveAccessFile();
	bool addUser(const QString &login, const QString &pass, const uint8_t group = UserGrpup::users);
	bool findUser(const QString &login);
	bool isSocks4Access(const QHostAddress &addr);
	bool chkAuth(const QString &login, const QString &pass);
	bool chkAuth2(const QString &login, const QString &pass);
	User getUserData(const QString &login);
	bool isBan(const QHostAddress &addr);
	void addBAN(const QHostAddress &addr, const uint8_t timeout = 60);
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
	uint8_t getTimeBan(const QHostAddress &addr);
	void addUserConnection(const QString &login, const QHostAddress &addr, const uint16_t port);
	void removeUserConnection(const QString &login, const QHostAddress &addr, const uint16_t port);
}

#endif // GLOBAL_H
