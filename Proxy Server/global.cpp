#include "global.h"
#include "myfunctions.h"

#include <QDateTime>
#include <QSettings>
#include <QHostInfo>

namespace app {
	Config conf;
	AccessList accessList;

	void loadSettings()
	{
		if( app::conf.confFile.isEmpty() ) return;

		app::setLog( 3, QString("LOAD SETTINGS [%1]...").arg(app::conf.confFile) );

		QSettings settings(app::conf.confFile, QSettings::IniFormat);

		app::conf.port = settings.value("MY_PROXY/port", app::conf.port).toUInt();
		app::conf.controlPort = settings.value("MY_PROXY/controlPort", app::conf.controlPort).toUInt();
		app::conf.accessFile = settings.value("MY_PROXY/accessFile", app::conf.accessFile).toString();
		app::conf.usersFile = settings.value("MY_PROXY/usersFile", app::conf.usersFile).toString();
		app::conf.logFile = settings.value("MY_PROXY/logFile" ,app::conf.logFile).toString();
		app::conf.logLevel = settings.value("MY_PROXY/logLevel", app::conf.logLevel).toUInt();
		app::conf.verbose = settings.value("MY_PROXY/verbose", app::conf.verbose).toBool();
		auto httpAuthMethod = settings.value("MY_PROXY/httpAuthMethod", "digest").toString();
		app::conf.httpAuthMethod = app::getHttpAuthMethodFromString( httpAuthMethod );


#ifdef __linux__

#elif _WIN32
	QDir dir(QDir::homePath());
	dir.mkdir("MyProxy");
#endif

		app::loadAccessFile();

		app::loadUsers();

		if( app::conf.page.top.size() == 0 )			app::loadResource( ":/assets/top.html", app::conf.page.top );
		if( app::conf.page.bottom.size() == 0 )			app::loadResource( ":/assets/bottom.html", app::conf.page.bottom );
		if( app::conf.page.menu.size() == 0 )			app::loadResource( ":/assets/menu.html", app::conf.page.menu );
		if( app::conf.page.index.size() == 0 )			app::loadResource( ":/assets/index.html", app::conf.page.index );
		if( app::conf.page.state.size() == 0 )			app::loadResource( ":/assets/state.html", app::conf.page.state );
		if( app::conf.page.admin.size() == 0 )			app::loadResource( ":/assets/admin.html", app::conf.page.admin );

		if( app::conf.page.buttonsCSS.size() == 0 )		app::loadResource( ":/assets/buttons.css", app::conf.page.buttonsCSS );
		if( app::conf.page.colorCSS.size() == 0 )		app::loadResource( ":/assets/color.css", app::conf.page.colorCSS );
		if( app::conf.page.indexCSS.size() == 0 )		app::loadResource( ":/assets/index.css", app::conf.page.indexCSS );

		if( app::conf.page.indexJS.size() == 0 )		app::loadResource( ":/assets/index.js", app::conf.page.indexJS );

		if( app::conf.page.downArrowIMG.size() == 0 )	app::loadResource( ":/assets/down-arrow.png", app::conf.page.downArrowIMG );
		if( app::conf.page.upArrowIMG.size() == 0 )		app::loadResource( ":/assets/up-arrow.png", app::conf.page.upArrowIMG );

		if( settings.allKeys().size() == 0 ) app::conf.settingsSave = true;
	}

	void saveSettings()
	{
		if( app::conf.confFile.isEmpty() ) return;

		if( app::conf.settingsSave ){
			app::setLog( 3, QString("SAVE SETTINGS [%1]...").arg(app::conf.confFile) );
			QSettings settings(app::conf.confFile, QSettings::IniFormat);
			settings.clear();
			settings.setValue("MY_PROXY/port",app::conf.port);
			settings.setValue("MY_PROXY/controlPort",app::conf.controlPort);
			settings.setValue("MY_PROXY/accessFile",app::conf.accessFile);
			settings.setValue("MY_PROXY/usersFile",app::conf.usersFile);
			settings.setValue("MY_PROXY/logFile",app::conf.logFile);
			settings.setValue("MY_PROXY/logLevel",app::conf.logLevel);
			settings.setValue("MY_PROXY/verbose",app::conf.verbose);
			settings.setValue("MY_PROXY/httpAuthMethod", app::getHttpAuthMethodFromCode( app::conf.httpAuthMethod ) );

			app::conf.settingsSave = false;
		}

		if( app::accessList.accessFileSave ) app::saveAccessFile();
		if( app::conf.usersSave ) app::saveUsers();
	}

	bool parsArgs(int argc, char *argv[])
	{
		bool ret = true;
		for(int i=0;i<argc;i++){
			if(QString(argv[i]).indexOf("-")==0){
				if(QString(argv[i]) == "--help" or QString(argv[1]) == "-h"){
					printf("Usage: %s [OPTIONS]\n"
							"  -l <FILE>    log file\n"
						   "  -c <FILE>    config file\n"
							"  -v    Verbose output\n"
							"\n", argv[0]);
					ret = false;
				}
				if(QString(argv[i]) == "-l") app::conf.logFile = QString(argv[++i]);
				if(QString(argv[i]) == "-c") app::conf.confFile = QString(argv[++i]);
				if(QString(argv[i]) == "-v") app::conf.verbose = true;
			}
		}
		return ret;
	}

	void setLog(const uint8_t logLevel, const QString &mess)
	{
		if(app::conf.logLevel < logLevel or app::conf.logLevel == 0) return;

		QDateTime dt = QDateTime::currentDateTime();
		QString str = dt.toString("yyyy.MM.dd [hh:mm:ss] ") + mess + "\n";

		if( app::conf.verbose ){
			printf( "%s", str.toUtf8().data() );
			fflush( stdout );
		}

		if( app::conf.logFile.isEmpty() ) return;
		QFile f;
		f.setFileName( app::conf.logFile );
		if( f.open( QIODevice::Append ) ){
			f.write( str.toUtf8() );
			f.close();
		}
	}

	void loadBlackList(const QString &fileName, std::vector<QString> &data)
	{
		app::setLog( 3, "LOAD BLACK ADDRS LIST..." );
		data.clear();
		QFile file;
		file.setFileName( fileName );
		if(file.open(QIODevice::ReadOnly | QIODevice::Text)){
			QByteArray str;
			char sym;
			while(!file.atEnd()){
				file.read( &sym, 1 );
				if( sym == '\n' ){
					data.push_back( str );
					str.clear();
					continue;
				}
				str.append( sym );
			}
			file.close();
		}
	}

	void saveBlackList(const QString &fileName, const std::vector<QString> &data)
	{
		app::setLog( 4, "SAVE BLACK ADDRS LIST..." );
		QFile file;
		file.setFileName( fileName );
		if(file.open(QIODevice::WriteOnly | QIODevice::Text)){
			for( auto elem:data ){
				file.write( elem.toLatin1() );
				file.write( "\n" );
			}
			file.close();
		}
	}

	void addGlobalBlackAddr(const QString &str)
	{
		bool find = false;

		for( auto elem:app::accessList.blackDomains ){
			if( elem == str ){
				find = true;
				break;
			}
		}

		if( !find ){
			app::accessList.blackDomains.push_back( str );
			app::accessList.accessFileSave = true;
			app::updateBlackWhiteDomains();
		}
	}

	void addGlobalBlackIP(const Host &host)
	{
		bool find = false;

		for( auto elem:app::accessList.blackIPs ){
			if( elem.ip == host.ip && elem.port == host.port ){
				find = true;
				break;
			}
		}

		if( !find ){
			app::accessList.blackIPs.push_back( host );
			app::accessList.accessFileSave = true;
		}
	}

	void addGlobalBlackIPDynamic(const Host &host)
	{
		bool find = false;

		for( auto elem:app::accessList.blackIPsDynamic ){
			if( elem.ip == host.ip && elem.port == host.port ){
				find = true;
				break;
			}
		}

		if( !find ) app::accessList.blackIPsDynamic.push_back( host );
	}

	void updateBlackWhiteDomains()
	{
		app::setLog( 4, "UPDATE BLACK IP LIST..." );
		app::updateListFromList( app::accessList.blackDomains, app::accessList.blackIPsDynamic );
		app::updateListFromList( app::accessList.whiteDomains, app::accessList.whiteIPs );
	}

	bool isBlockHost(const Host& host)
	{
		bool res = false;

		for( auto elem:app::accessList.blackIPs ){
			if( elem.ip.toString() == "0.0.0.0" ){
				if( host.port == elem.port || ( host.port == 0 || elem.port == 0 ) ){
					res = true;
					break;
				}
			}
			if( elem.ip == host.ip ){
				if( elem.port == host.port || ( elem.port == 0 || host.port == 0 ) ){
					res = true;
					break;
				}
			}
		}

		if( !res ){
			for( auto elem:app::accessList.blackIPsDynamic ){
				if( elem.ip.toString() == "0.0.0.0" ){
					if( host.port == elem.port || ( host.port == 0 || elem.port == 0 ) ){
						res = true;
						break;
					}
				}
				if( elem.ip == host.ip ){
					if( elem.port == host.port || ( elem.port == 0 || host.port == 0 ) ){
						res = true;
						break;
					}
				}
			}
		}

		if( res ){
			//Если заблокирован то проверим белый список
			for( auto elem:app::accessList.whiteIPs ){
				if( elem.ip.toString() == "0.0.0.0" ){
					if( host.port == elem.port || ( host.port == 0 || elem.port == 0 ) ){
						res = false;
						break;
					}
				}
				if( elem.ip == host.ip ){
					if( elem.port == host.port || ( elem.port == 0 || host.port == 0 ) ){
						res = false;
						break;
					}
				}
			}
		}

		app::setLog( 5, QString("isBlockHost [%1:%2] [%3]").arg( host.ip.toString() ).arg( host.port ).arg( (res)?"true":"false" ) );

		return res;
	}

	void loadAccessFile()
	{
		if( app::conf.accessFile.isEmpty() ){
			app::setLog( 3, QString("LOAD ACCESS FILE [%1] ... ERROR not defined").arg( app::conf.accessFile ) );
			return;
		}

		if( !mf::checkFile( app::conf.accessFile.toUtf8().data() ) ){
			app::setLog( 3, QString("LOAD ACCESS FILE [%1] ... ERROR not found").arg( app::conf.accessFile ) );
			return;
		}

		app::setLog( 3, QString("LOAD ACCESS FILE... [%1]").arg( app::conf.accessFile ) );

		app::accessList.banList.clear();
		app::accessList.blackDomains.clear();
		app::accessList.blackIPs.clear();
		app::accessList.socks4Access.clear();
		app::accessList.whiteDomains.clear();
		app::accessList.whiteIPs.clear();

		bool BANipAddrs		= false;
		bool blackDomains	= false;
		bool blackIPs		= false;
		bool socks4Access	= false;
		bool whiteDomains	= false;
		bool whiteIPs		= false;
		QStringList whiteIPsList;
		QStringList blackIPsList;

		QHostAddress addr;

		QFile file;
		file.setFileName( app::conf.accessFile );
		if( file.open( QIODevice::ReadOnly ) ){
			while(!file.atEnd()){
				auto str = file.readLine();
				str.replace( "\n", "" );
				str.replace( "\r", "" );
				if( str == "[Socks4Access]" ){
					socks4Access	= true;
					BANipAddrs		= false;
					blackDomains	= false;
					blackIPs		= false;
					whiteDomains	= false;
					whiteIPs		= false;
					continue;
				}
				if( str == "[WhiteDomains]" ){
					socks4Access	= false;
					BANipAddrs		= false;
					blackDomains	= false;
					blackIPs		= false;
					whiteDomains	= true;
					whiteIPs		= false;
					continue;
				}
				if( str == "[WhiteAddrs]" ){
					socks4Access	= false;
					BANipAddrs		= false;
					blackDomains	= false;
					blackIPs		= false;
					whiteDomains	= false;
					whiteIPs		= true;
					continue;
				}
				if( str == "[BlackDomains]" ){
					socks4Access	= false;
					BANipAddrs		= false;
					blackDomains	= true;
					blackIPs		= false;
					whiteDomains	= false;
					whiteIPs		= false;
					continue;
				}
				if( str == "[BlackAddrs]" ){
					socks4Access	= false;
					BANipAddrs		= false;
					blackDomains	= false;
					blackIPs		= true;
					whiteDomains	= false;
					whiteIPs		= false;
					continue;
				}
				if( str == "[BanIPs]" ){
					socks4Access	= false;
					BANipAddrs		= true;
					blackDomains	= false;
					blackIPs		= false;
					whiteDomains	= false;
					whiteIPs		= false;
					continue;
				}
				if( socks4Access ){
					if( addr.setAddress( QString(str) ) ) app::accessList.socks4Access.push_back( addr );
				}
				if( BANipAddrs ){
					auto elem = str.split('=');
					if( elem.size() == 2 ){
						if( addr.setAddress( QString(elem[0]) ) ) app::addBAN( addr, elem[1].toHex().toUInt( nullptr, 16 ) );
					}
				}
				if( whiteIPs ) whiteIPsList.push_back( QString(str) );
				if( blackIPs ) blackIPsList.push_back( QString(str) );
				if( blackDomains ) app::accessList.blackDomains.push_back( str );
				if( whiteDomains ) app::accessList.whiteDomains.push_back( str );
			}
			file.close();

			app::updateListFromList( whiteIPsList, app::accessList.whiteIPs );
			app::updateListFromList( blackIPsList, app::accessList.blackIPs );

			app::updateBlackWhiteDomains();
		}
	}

	void saveAccessFile()
	{
		if( app::conf.accessFile.isEmpty() ){
			app::setLog( 3, QString("SAVE ACCESS FILE [%1] ... ERROR not defined").arg( app::conf.accessFile ) );
			return;
		}

		app::setLog( 4, QString("SAVE ACCESS FILE [%1] ...").arg( app::conf.accessFile ) );

		QStringList list;

		QFile file;
		file.setFileName( app::conf.accessFile );
		if( file.open( QIODevice::WriteOnly ) ){
			file.write("[Socks4Access]\n");
			for( auto elem:app::accessList.socks4Access ){
				file.write( elem.toString().toUtf8().data() );
				file.write("\n");
			}
			file.write("[WhiteAddrs]\n");
			list.clear();
			app::updateListFromList( app::accessList.whiteIPs, list );
			file.write( list.join("\n").toUtf8().data() );
			file.write("[WhiteDomains]\n");
			for( auto elem:app::accessList.whiteDomains ){
				file.write( elem.toUtf8().data() );
				file.write("\n");
			}
			file.write("[BlackAddrs]\n");
			list.clear();
			app::updateListFromList( app::accessList.blackIPs, list );
			file.write( list.join("\n").toUtf8().data() );
			file.write("[BlackDomains]\n");
			for( auto elem:app::accessList.blackDomains ){
				file.write( elem.toUtf8().data() );
				file.write("\n");
			}
			file.write("[BanIPs]\n");
			for( auto elem:app::accessList.banList ){
				file.write( elem.addr.toString().toUtf8().data() );
				file.write("=");
				file.write( QString::number(elem.sec).toUtf8().data() );
				file.write("\n");
			}
			file.close();
		}

		app::accessList.accessFileSave = false;
	}

	bool addUser(const QString &login, const QString &pass, const uint8_t group)
	{
		bool res = false;

		if( !findUser( login ) ){
			User user;
			user.login = login;
			user.pass = mf::md5( login.toUtf8() + ":" + app::conf.realmString + ":" + pass.toUtf8() ).toHex();
			user.group = group;
			app::conf.users.push_back( user );
			app::conf.usersSave = true;
			res = true;
		}

		return res;
	}

	bool findUser(const QString &login)
	{
		bool res = false;

		for( auto user:app::conf.users ){
			if( login == user.login ){
				res = true;
				break;
			}
		}

		return res;
	}

	bool isSocks4Access(const QHostAddress &addr)
	{
		bool res = false;

		for( auto accessAddr:app::accessList.socks4Access ){
			if( addr == accessAddr ){
				res = true;
				break;
			}
		}

		return res;
	}

	bool chkAuth(const QString &login, const QString &pass)
	{
		bool res = false;

		for( auto user:app::conf.users ){
			if( login == user.login ){
				auto newPass = mf::md5( login.toUtf8() + ":" + app::conf.realmString + ":" + pass.toUtf8() ).toHex();
				if( newPass == user.pass ) res = true;
				break;
			}
		}

		return res;
	}

	bool chkAuth2(const QString &login, const QString &pass)
	{
		bool res = false;

		for( auto user:app::conf.users ){
			if( login == user.login ){
				if( pass == user.pass ) res = true;
				break;
			}
		}

		return res;
	}

	User getUserData(const QString &login)
	{
		User data;

		for( auto user:app::conf.users ){
			if( login == user.login ){
				data = user;
				break;
			}
		}

		return data;
	}

	bool isBan(const QHostAddress &addr)
	{
		bool res = false;

		for( auto elem:app::accessList.banList ){
			if( addr == elem.addr ){
				res = true;
				break;
			}
		}

		return res;
	}

	void addBAN(const QHostAddress &addr, const uint8_t timeout)
	{
		if( !isBan( addr ) ){
			BanData data;
			data.addr = addr;
			data.sec = timeout;

			app::setLog( 2, QString("add to BAN [%1] ...").arg( addr.toString() ) );

			app::accessList.banList.push_back( data );
			app::accessList.accessFileSave = true;
			return;
		}else{
			for( auto &elem:app::accessList.banList ){
				if( addr == elem.addr ){
					elem.sec += timeout;
					break;
				}
			}
		}
	}

	bool isBlockedDomName(const QString &domName)
	{
		bool res = false;

		for( auto elem:app::accessList.blackDomains ){
			if( mf::strFind( elem, domName ) ){
				res = true;
				break;
			}
		}

		if( res ){
			// Если заблокирован, проверяем в белом списке
			for( auto elem:app::accessList.whiteDomains ){
				if( mf::strFind( elem, domName ) ){
					res = true;
					break;
				}
			}
		}

		app::setLog( 5, QString("isBlockedDomName [%1] [%2]").arg( domName ).arg( res ) );

		return res;
	}

	void loadUsers()
	{
		if( app::conf.usersFile.isEmpty() ){
			app::setLog( 2, QString("LOAD USER FILE [%1] ... ERROR not defined").arg( app::conf.usersFile ) );
			return;
		}

		if( !mf::checkFile( app::conf.usersFile.toUtf8().data() ) ){
			app::setLog( 2, QString("LOAD USER FILE [%1] ... ERROR not found").arg( app::conf.usersFile ) );
			return;
		}

		app::setLog( 3, QString("LOAD USER FILE [%1] ...").arg( app::conf.usersFile ) );

		app::conf.users.clear();

		QSettings users( app::conf.usersFile, QSettings::IniFormat );

		for( auto group:users.childGroups() ){
			app::setLog( 4, QString("   FOUND USER [%1]").arg( group ) );
			users.beginGroup( group );

			User user;

			user.login				= group;
			user.group				= app::getUserGroupFromName( users.value( "group", "" ).toString() );
			user.pass				= users.value( "password", "" ).toString();
			user.maxConnections		= users.value( "maxConnections", user.maxConnections ).toUInt();
			user.bytesMax			= users.value( "bytesMax", user.bytesMax ).toUInt();
			auto accessList			= users.value( "accessList", "*" ).toString().split(",");
			auto blockList			= users.value( "blockList", "*" ).toString().split(",");

			app::updateListFromList( accessList, user.accessList );
			app::updateListFromList( blockList, user.blockList );

			app::conf.users.push_back( user );

			users.endGroup();
		}
	}

	void saveUsers()
	{
		if( app::conf.usersFile.isEmpty() ){
			app::setLog( 2, QString("SAVE USER FILE [%1] ... ERROR not defined").arg( app::conf.usersFile ) );
			return;
		}

		app::setLog( 3, QString("SAVE USER FILE [%1] ...").arg( app::conf.usersFile ) );

		QSettings users( app::conf.usersFile, QSettings::IniFormat );
		users.clear();

		for( auto user:app::conf.users ){
			users.beginGroup( user.login );

			users.setValue( "group", app::getUserGroupNameFromID( user.group ) );
			users.setValue( "password", user.pass );
			users.setValue( "maxConnections", user.maxConnections );
			QStringList accessList;
			QStringList blockList;
			app::updateListFromList( user.accessList, accessList );
			app::updateListFromList( user.blockList, blockList );
			users.setValue( "accessList", accessList.join(",") );
			users.setValue( "blockList", blockList.join(",") );
			users.setValue( "bytesMax", user.bytesMax );

			users.endGroup();
		}

		app::conf.usersSave = false;
	}

	bool isBlockedToUser(const QString &login, const Host &host)
	{
		bool res = false;

		for( auto elem:app::getUserData( login ).blockList ){
			if( elem.ip.toString() == "0.0.0.0" ){
				if( host.port == elem.port || ( host.port == 0 || elem.port == 0 ) ){
					res = true;
					break;
				}
			}
			if( host.ip == elem.ip ){
				if( host.port == elem.port || ( host.port == 0 || elem.port == 0 ) ){
					res = true;
					break;
				}
			}
		}

		if( res ){
			//Если заблокирован то проверим белый список
			for( auto elem:app::getUserData( login ).accessList ){
				if( elem.ip.toString() == "0.0.0.0" ){
					if( host.port == elem.port || ( host.port == 0 || elem.port == 0 ) ){
						res = false;
						break;
					}
				}
				if( host.ip == elem.ip ){
					if( host.port == elem.port || ( host.port == 0 || elem.port == 0 ) ){
						res = false;
						break;
					}
				}
			}
		}

		app::setLog( 4, QString("isBlockedToUser [%1:%2] [%3]").arg( host.ip.toString() ).arg( host.port ).arg( (res)?"true":"false" ) );

		return res;
	}

	void addUserConnection(const QString &login, const QString &host)
	{
		app::setLog( 5, QString("addUserConnection [%1] [%3]").arg( login ).arg( host ) );
		auto iter = app::conf.usersConnections.find( login );
		if( iter != app::conf.usersConnections.end() ) (*iter).second.push_back( host );
	}

	void removeUserConnection(const QString &login, const QString &host)
	{
		app::setLog( 5, QString("removeUserConnection [%1] [%3]").arg( login ).arg( host ) );
		auto iter = app::conf.usersConnections.find( login );
		if( iter != app::conf.usersConnections.end() ) (*iter).second.removeOne( host );
	}

	uint32_t getUserConnectionsNum(const QString &login)
	{
		uint32_t num = 0;

		auto iter = app::conf.usersConnections.find( login );
		if( iter != app::conf.usersConnections.end() ) num = static_cast<uint32_t>( (*iter).second.size() );

		return num;
	}

	uint8_t getUserGroupFromName(const QString &name)
	{
		uint8_t res = UserGrpup::users;

		if( name == "admin" ){
			res = UserGrpup::admins;
			return res;
		}

		return res;
	}

	QString getUserGroupNameFromID(const uint8_t id)
	{
		QString res = "user";

		switch( id ){
			case UserGrpup::admins:		res = "admin";	break;
		}

		return res;
	}

	void updateUserLoginTimeStamp(const QString &login)
	{
		for( auto &user:app::conf.users ){
			if( login == user.login ){
				user.lastLoginTimestamp = QDateTime::currentDateTime().toTime_t();
				break;
			}
		}
	}

	void updateBanList()
	{
		QList<int> removeList;
		int i = 0;

		for( auto &elem:app::accessList.banList ){
			if( elem.sec > 0 ){
				elem.sec--;
			}else{
				removeList.push_back( i );
			}
			i++;
		}

		if( removeList.size() > 0 ){
			for( auto elem:removeList ) app::accessList.banList.removeAt( elem );
			app::accessList.accessFileSave = true;
		}
	}

	uint8_t getTimeBan(const QHostAddress &addr)
	{
		uint8_t res = 0;

		for( auto elem:app::accessList.banList ){
			if( addr == elem.addr ){
				res = elem.sec;
				break;
			}
		}

		return res;
	}

	void getIPsFromDomName(const QString &domName, const uint16_t port, std::vector<Host> &data)
	{
		auto info = QHostInfo::fromName( domName );
		if( info.error() == QHostInfo::NoError ){
			for(auto elem:info.addresses()){
				app::setLog( 4, QString("getIPsFromDomName found ip [%1] ...").arg( elem.toString() ) );
				Host host;
				host.port = port;
				host.ip = elem;
				data.push_back( host );
			}
		}
	}

	void getIPFromDomName(const QString &domName, Host &host)
	{
		auto info = QHostInfo::fromName( domName );
		if( info.error() == QHostInfo::NoError ){
			if( info.addresses().size() > 0 ){
				host.ip = info.addresses().at( 0 );
			}else{
				host.ip = QHostAddress( "255.255.255.255" );
			}
		}
	}

	void updateListFromList(const QStringList &list, std::vector<Host> &data)
	{
		for( auto elem:list ){
			auto tmp = elem.split(":");
			QString ip = tmp[0];
			if( ip == "*" ) ip = "0.0.0.0";
			app::setLog( 4, QString("Update list ips [%1] ...").arg( elem ) );
			Host host;
			if( tmp.size() == 2 ){
				if( tmp[1] == "*" || tmp[1] == "0" ){
					host.port = 0;
				}else{
					host.port = tmp[1].toUShort();
				}
			}
			if( ip.indexOf("*.") == 0 ) ip = ip.right( ip.length() - 2 );
			if( !host.ip.setAddress( ip ) ){
				app::setLog( 4, QString("Update list ips is domainName [%1]").arg( host.port ) );
				app::getIPsFromDomName( ip, host.port, data );
				continue;
			}
			app::setLog( 4, QString("Update list ips added [%1:%2] ...").arg( host.ip.toString() ).arg( host.port ) );
			data.push_back( host );
		}
	}

	void updateListFromList(const std::vector<Host> &data, QStringList &list)
	{
		for( auto elem:data ) list.push_back( QString("%1:%2").arg( elem.ip.toString() ).arg( elem.port ) );
	}

	QString getHtmlPage(const QString &content)
	{
		QString str;

		str += "<!DOCTYPE html><html lang=\"en\">";
		str += "<head>";
		str += "<meta charset=\"utf-8\"/>";
		str += "<title>-= Proxy Server service page =-</title>";
		str += "</head>";
		str += "<body>";

		str += content;

		str += "</body>";
		str += "</html>";

		return str;
	}

	void addBytesInTraffic(const QString &host, const QString &login, const uint32_t bytes)
	{
		if( bytes == 0 ) return;
		if( host == "127.0.0.1" ) return;
		for( auto &user:app::conf.users ){
			if( login == user.login ){
				user.inBytes += bytes;
				break;
			}
		}
	}

	void addBytesOutTraffic(const QString &host, const QString &login, const uint32_t bytes)
	{
		if( bytes == 0 ) return;
		if( host == "127.0.0.1" ) return;
		for( auto &user:app::conf.users ){
			if( login == user.login ){
				user.outBytes += bytes;
				break;
			}
		}
	}

	void loadResource(const QString &fileName, QByteArray &data)
	{
		data.clear();
		QFile file;

		file.setFileName( fileName );
		if(file.open(QIODevice::ReadOnly)){
			while (!file.atEnd()) data.append( file.readAll() );
			file.close();
		}
	}

	QByteArray getHtmlPage(const QByteArray &title, const QByteArray &content)
	{
		QByteArray ba;
			ba.append( app::conf.page.top );
			ba.append( "		<title>-= " );
			ba.append( title );
			ba.append( " =-</title>\n	</head>\n<body>\n" );
			ba.append( app::conf.page.menu );
			ba.append( content );
			ba.append( app::conf.page.bottom );
			return ba;
	}

	uint8_t getHttpAuthMethodFromString(const QString &string)
	{
		if( string.toLower() == "basic" ) return http::AuthMethod::Basic;
		if( string.toLower() == "digest" ) return http::AuthMethod::Digest;
		if( string.toLower() == "ntlm" ) return http::AuthMethod::NTLM;

		return http::AuthMethod::Unknown;
	}

	QString getHttpAuthMethodFromCode(const uint8_t code)
	{
		QString str;

		switch (code) {
			case http::AuthMethod::Basic: str = "Basic"; break;
			case http::AuthMethod::Digest: str = "Digest"; break;
			case http::AuthMethod::NTLM: str = "NTLM"; break;
			default: str = "unknown"; break;
		}

		return str;
	}

	QByteArray getNonceCode()
	{
		QDateTime dt = QDateTime::currentDateTime();
		QByteArray dtStr;
		dtStr.append( dt.toString("yyyy.MM.dd") );
		dtStr.append( ":->" );
		dtStr.append( app::conf.realmString );
		return mf::md5(dtStr).toHex();
	}

	QByteArray getHA1Code(const QString &login)
	{
		QByteArray HA1;

		for( auto user:app::conf.users ){
			if( login == user.login ){
				HA1.append( user.pass );
				break;
			}
		}

		return HA1;
	}

	QString getHttpAuthString()
	{
		QString str;

		switch (app::conf.httpAuthMethod){
			case  http::AuthMethod::Basic:	str += "Basic realm=\"" + app::conf.realmString + "\"";		break;
			case  http::AuthMethod::Digest:
				//str += "Digest realm=\"" + app::conf.realmString + "\", qop=\"auth,auth-int\", nonce=\"" + mf::md5(dtStr).toHex() + "\",opaque=\"" + mf::md5(app::conf.realmString).toHex() + "\"";
				str += "Digest realm=\"" + app::conf.realmString + "\", qop=auth, nonce=\"" + app::getNonceCode() + "\",opaque=\"" + mf::md5(app::conf.realmString).toHex() + "\"";
				//str += "Digest realm=\"" + app::conf.realmString + "\", nonce=\"" + app::getNonceCode() + "\",opaque=\"" + mf::md5(app::conf.realmString).toHex() + "\"";
			break;
		}

		return str;
	}

	bool isMaxConnections(const QString &login)
	{
		bool res = true;

		uint32_t maxConnections = 0;

		for( auto user:app::conf.users ){
			if( login == user.login ){
				maxConnections = user.maxConnections;
				break;
			}
		}

		if( maxConnections > app::getUserConnectionsNum( login ) ) res = false;

		return res;
	}

	QString getUserPass(const QString &login)
	{
		QString res;

		for( auto user:app::conf.users ){
			if( login == user.login ){
				res = user.pass;
				break;
			}
		}

		return res;
	}

	bool isTrafficLimit(const QString &login)
	{
		bool res = false;

		for( auto user:app::conf.users ){
			if( login == user.login ){
				if( user.bytesMax == 0 ) break;
				if( ( user.inBytes + user.outBytes ) >= user.bytesMax ) res = true;
				break;
			}
		}

		return res;
	}

	QString getDateTime(const uint32_t timeStamp)
	{
		QDateTime dt;
		dt.setTime_t( timeStamp );
		QString str = dt.toString("yyyy.MM.dd [hh:mm:ss] ");
		return str;
	}

	bool changePassword(const QString &login, const QString &newPassword)
	{
		bool res = false;

		for( auto &user:app::conf.users ){
			if( login == user.login ){
				user.pass = mf::md5( login.toUtf8() + ":" + app::conf.realmString + ":" + newPassword.toUtf8() ).toHex();
				app::conf.usersSave = true;
				res = true;
				break;
			}
		}

		return res;
	}

	bool changeMaxConnections(const QString &login, const uint32_t maxConnections)
	{
		bool res = false;

		for( auto &user:app::conf.users ){
			if( login == user.login ){
				user.maxConnections = maxConnections;
				app::conf.usersSave = true;
				res = true;
				break;
			}
		}

		return res;
	}

	bool changeMaxBytes(const QString &login, const uint32_t bytesMax)
	{
		bool res = false;

		for( auto &user:app::conf.users ){
			if( login == user.login ){
				user.bytesMax = bytesMax;
				app::conf.usersSave = true;
				res = true;
				break;
			}
		}

		return res;
	}

	void removeGlobalBlackAddr(const QString &str)
	{
		if( app::accessList.blackDomains.removeAll( str ) ){
			app::accessList.accessFileSave = true;
			app::updateBlackWhiteDomains();
		}
	}

	void addSocks4AccessIP(const QString &str)
	{
		QHostAddress ip;
		if( !ip.setAddress( str ) ) return;
		app::accessList.socks4Access.push_back( ip );
		app::accessList.accessFileSave = true;
	}

	void removeSocks4AccessIP(const QString &str)
	{
		QHostAddress ip;
		if( !ip.setAddress( str ) ) return;
		if( app::accessList.socks4Access.removeAll( ip ) ) app::accessList.accessFileSave = true;
	}

}
