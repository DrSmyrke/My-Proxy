#ifndef GLOBAL_H
#define GLOBAL_H

#include <QString>
#include <vector>

#include "http.h"
#include "myfunctions.h"

struct UserGrpups{
	enum{
		users,
		admins,
	};
};

struct User{
	QString login;
	QString pass;
	uint8_t group = UserGrpups::users;
};

struct Config{
	bool verbose						= false;
	uint8_t logLevel					= 3;
	QString logFile						= "/tmp/webProxy.log";
	uint8_t maxThreads					= 3;
	uint8_t maxClients					= 37;
	uint16_t port						= 7300;
	std::vector<QString> socksClients;
	std::vector<User> users;
	uint8_t authMetod					= http::AuthMethod::Basic;
	QString codeWord					= "DeadBeef";
};

namespace app {
	extern Config conf;

	void loadSettings();
	void saveSettings();
	bool parsArgs(int argc, char *argv[]);
	void setLog(const uint8_t logLevel, const QString &mess);
	QString getHtmlPage(const QString &title, const QString &content);
	bool addUser(const QString &login, const QString &pass);
	bool passIsValid(const QString &pass, const QString &hash);
}

#endif // GLOBAL_H
