#ifndef GLOBAL_H
#define GLOBAL_H

#include <QString>

enum ProxyType{
	HTTP,
	HTTPS,
	SOCKS4,
	SOCKS5,
};

struct Config{
	bool verbose			= false;
	uint8_t logLevel		= 3;
	QString logFile			= "/tmp/webProxy.log";
	uint8_t maxThreads		= 3;
	uint8_t maxClients		= 37;
	uint16_t port			= 7300;
};

namespace app {
	extern Config conf;

	void loadSettings();
	void saveSettings();
	bool parsArgs(int argc, char *argv[]);
	void setLog(const uint8_t logLevel, const QString &mess);
}

#endif // GLOBAL_H
