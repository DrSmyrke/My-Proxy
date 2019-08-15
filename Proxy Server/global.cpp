#include "global.h"

#include <QDateTime>
#include <QSettings>
#include <QUrl>



namespace app {
	Config conf;
	Status state;
	BlackList blackList;

	void loadSettings()
	{
		QSettings settings("MySoft","MyProxy");

		app::conf.maxThreads = settings.value("WEB PROXY/maxThreads",app::conf.maxThreads).toUInt();
		app::conf.port = settings.value("WEB PROXY/port",app::conf.port).toUInt();
		app::conf.blackUrlsFile = settings.value("WEB PROXY/blackUrlsFile",app::conf.blackUrlsFile).toString();
		app::conf.blackAddrsFile = settings.value("WEB PROXY/blackAddrsFile",app::conf.blackAddrsFile).toString();
		app::conf.logFile = settings.value("WEB PROXY/logFile",app::conf.logFile).toString();
		app::conf.logLevel = settings.value("WEB PROXY/logLevel",app::conf.logLevel).toUInt();


		app::loadResource( ":/pages/assets/top.html", app::conf.page.top );
		app::loadResource( ":/pages/assets/bottom.html", app::conf.page.bottom );
		app::loadResource( ":/pages/assets/menu.html", app::conf.page.menu );
		app::loadResource( ":/pages/assets/index.js", app::conf.page.indexJS );
		app::loadResource( ":/pages/assets/color.css", app::conf.page.colorCSS );
		app::loadResource( ":/pages/assets/index.css", app::conf.page.indexCSS );
		app::loadResource( ":/pages/assets/buttons.css", app::conf.page.buttonsCSS );
		app::loadResource( ":/pages/assets/admin.html", app::conf.page.admin );
		app::loadResource( ":/pages/assets/state.html", app::conf.page.state );
		app::loadResource( ":/pages/assets/config.html", app::conf.page.config );
		app::loadResource( ":/pages/assets/index.html", app::conf.page.index );

		app::loadBlackList( app::conf.blackUrlsFile, app::blackList.urls );
		app::loadBlackList( app::conf.blackAddrsFile, app::blackList.addrs );

		app::addGlobalBlackUrl("*/ad");
		app::addGlobalBlackUrl("ad??.*");
		app::addGlobalBlackUrl("ad?.*");
		app::addGlobalBlackUrl("ad.*");
		app::addGlobalBlackUrl("*adframe*");
		app::addGlobalBlackUrl("*/ad-handler");
		app::addGlobalBlackUrl("*/ads");
		app::addGlobalBlackUrl("ads???.*");
		app::addGlobalBlackUrl("ads.*");
		app::addGlobalBlackUrl("adserv.*");
		app::addGlobalBlackUrl("*/banner");
		app::addGlobalBlackUrl("*/popup");
		app::addGlobalBlackUrl("*/popups");

#ifdef __linux__

#elif _WIN32
	QDir dir(QDir::homePath());
	dir.mkdir("MyProxy");
#endif

	}

	void saveSettings()
	{
		if( app::conf.saveSettings ){
			QSettings settings("MySoft","MyProxy");
			settings.clear();
			settings.setValue("WEB PROXY/maxThreads",app::conf.maxThreads);
			settings.setValue("WEB PROXY/port",app::conf.port);
			settings.setValue("WEB PROXY/blackUrlsFile",app::conf.blackUrlsFile);
			settings.setValue("WEB PROXY/blackAddrsFile",app::conf.blackAddrsFile);
			settings.setValue("WEB PROXY/logFile",app::conf.logFile);
			settings.setValue("WEB PROXY/logLevel",app::conf.logLevel);

			app::conf.saveSettings = false;
		}
		if( app::blackList.urlsFileSave ){
			app::saveBlackList( app::conf.blackUrlsFile, app::blackList.urls );
			app::blackList.urlsFileSave = false;
		}
		if( app::blackList.addrsFileSave ){
			app::saveBlackList( app::conf.blackAddrsFile, app::blackList.addrs );
			app::blackList.addrsFileSave = false;
		}
	}

	bool parsArgs(int argc, char *argv[])
	{
		bool ret = true;
		for(int i=0;i<argc;i++){
			if(QString(argv[i]).indexOf("-")==0){
				if(QString(argv[i]) == "--help" or QString(argv[1]) == "-h"){
					printf("Usage: %s [OPTIONS]\n"
							"  -l <FILE>    log file\n"
							"  -v    Verbose output\n"
							"\n", argv[0]);
					ret = false;
				}
				if(QString(argv[i]) == "-l") app::conf.logFile = QString(argv[++i]);
				if(QString(argv[i]) == "-v") app::conf.verbose = true;
			//}else{
			//	bool ok = false;
			//	QString(argv[i]).toInt(&ok,10);
			//	if(ok) app::conf.port = QString(argv[i]).toInt();
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
		FILE* f;
		f = fopen(app::conf.logFile.toUtf8().data(),"a+");
		fwrite(str.toUtf8().data(),static_cast<size_t>(str.length()),1,f);
		fclose(f);
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

	bool addUser(const QString &login, const QString &pass, const uint8_t group)
	{
		bool res = true;
		User user;
		user.login = login;
		user.pass = mf::md5( login.toUtf8() + ":" + app::conf.realmString + ":" + pass.toUtf8() ).toHex();
		user.group = group;
		app::conf.users.push_back( user );
		return res;
	}

	bool chkAuth(const QString &login, const QString &pass)
	{
		bool res = false;
		for( auto &user:app::conf.users){
			if( user.login == login ){
				auto newPass = mf::md5( login.toUtf8() + ":" + app::conf.realmString + ":" + pass.toUtf8() ).toHex();
				if( newPass == user.pass ) res = true;
				break;
			}
		}
		return res;
	}

	void usersConnectionsClear()
	{
		for( auto &user:app::conf.users ) user.connections = 0;
	}

	void usersConnectionsNumAdd(const QString &login, const int32_t num)
	{
		for( auto &user:app::conf.users ){
			if( user.login == login ){
				int32_t r = user.connections + num;
				if( r >= 0 ) user.connections += num;
				break;
			}
		}
	}

	void addOpenUrl(const QUrl &url)
	{
		bool find = false;
		for( auto elem:app::state.urls ){
			if( elem == url.toString() ){
				find = true;
				break;
			}
		}
		if( !find ) app::state.urls.push_back( url.toString() );
	}

	void addOpenAddr(const QString &addr)
	{
		bool find = false;
		for( auto elem:app::state.addrs ){
			if( elem == addr ){
				find = true;
				break;
			}
		}
		if( !find ) app::state.addrs.push_back( addr );
	}

	void loadResource(const QString &fileName, QByteArray &data)
	{
		data.clear();
		QFile file;

		file.setFileName( fileName );
		if(file.open(QIODevice::ReadOnly | QIODevice::Text)){
			while (!file.atEnd()) data.append( file.readAll() );
			file.close();
		}
	}

	void loadBlackList(const QString &fileName, std::vector<QString> &data)
	{
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

	void addGlobalBlackUrl(const QString &str)
	{
		bool find = false;

		for( auto elem:app::blackList.urls ){
			if( elem == str ){
				find = true;
				break;
			}
		}

		if( !find ){
			app::blackList.urls.push_back( str );
			uint16_t i = 0;
			bool find = false;
			for( auto addr:app::state.urls ){
				if( addr == str ){
					find = true;
					break;
				}
				i++;
			}
			if( find ) app::state.urls.erase( app::state.urls.begin() + i );
			app::blackList.urlsFileSave = true;
		}
	}

	void addGlobalBlackAddr(const QString &str)
	{
		bool find = false;

		for( auto elem:app::blackList.addrs ){
			if( elem == str ){
				find = true;
				break;
			}
		}

		if( !find ){
			app::blackList.addrs.push_back( str );
			uint16_t i = 0;
			bool find = false;
			for( auto addr:app::state.addrs ){
				if( addr == str ){
					find = true;
					break;
				}
				i++;
			}
			if( find ) app::state.addrs.erase( app::state.addrs.begin() + i );
			app::blackList.addrsFileSave = true;
		}
	}

	QByteArray parsRequest(const QString &data, const User &userData)
	{
		QByteArray ba;
		QByteArray method;
		QByteArray tempBuff;
		QByteArray param;
		bool value = false;
		std::map< QByteArray, std::vector<QByteArray> > args;


		for( uint16_t i = 0; i < data.length(); i++ ){
			if( i == 0 && data[i] == '/' ) continue;
			if( i > 0 ){
				if( data[i] == '?' || data[i] == '=' || data[i] == '&' ){
					if( tempBuff.size() == 0 ) break;
				}
			}
			if( i > 0 && data[i] == '?' && method.size() == 0 && !value ){
				method.append( tempBuff );
				tempBuff.clear();
				continue;
			}

			if( i > 0 && data[i] == '=' && !value ){
				param.append( tempBuff );
				tempBuff.clear();
				continue;
			}

			if( i > 0 && data[i] == '"' && param.size() > 0 ){
				value = !value;
				continue;
			}

			if( i > 0 && data[i] == '&' && !value ){
				if( param.size() == 0 ) break;
				args[param].push_back( tempBuff );
				tempBuff.clear();
				param.clear();
				continue;
			}

			tempBuff.append( data[i] );
		}

		if( param.size() > 0 && tempBuff.size() > 0 ) args[param].push_back( tempBuff );

		ba.append( processingRequest( method, args, userData ) );

		return ba;
	}

	void getUserData(User &userData, const QString &login)
	{
		for( auto user:app::conf.users ){
			if( login == user.login ){
				userData = user;
				break;
			}
		}
	}

	QByteArray processingRequest(const QString &method, const std::map< QByteArray, std::vector<QByteArray> > &args, const User &userData)
	{
		QByteArray ba;

		if( method == "get" ){
			ba.append("content:>:");
			for( auto type:args ){
				for( auto value:type.second ){
					//qDebug()<<type.first<<value<<method;
					//"con" "globalBlockedUrls" "get"
					if( type.first == "con" && value == "globalBlockedUrls" ){
						ba.append("globalBlockedUrls:>:");
						ba.append("<table>");
						for( auto elem:app::blackList.urls ){
							ba.append("<tr>");
							ba.append( QString("<td>" + elem + "</td>") );
							ba.append("</tr>");
						}
						ba.append("</table>");
					}
					if( type.first == "con" && value == "globalBlockedAddrs" ){
						ba.append("globalBlockedAddrs:>:");
						ba.append("<table>");
						for( auto elem:app::blackList.addrs ){
							ba.append("<tr>");
							ba.append( QString("<td>" + elem + "</td>") );
							ba.append("</tr>");
						}
						ba.append("</table>");
					}
					if( type.first == "con" && value == "threads" ){
						ba.append("threads:>:");
						ba.append("<table>");
						for( uint8_t i = 0; i < app::state.threads.size(); i++ ){
							ba.append("<tr>");
							ba.append( QString("<td>Thread #" + QString::number( i ) + ":</td><td>" + QString::number( app::state.threads.at( i ) ) + " / " + QString::number( app::conf.maxClients ) + "</td>") );
							ba.append("</tr>");
						}
						ba.append("</table>");
					}
					if( type.first == "con" && value == "openUrls" ){
						ba.append("openUrls:>:");
						ba.append("<table>");
						for( auto url:app::state.urls ){
							ba.append("<tr>");
							QString adminB = ( userData.group == UserGrpup::admins ) ? "<input type=\"button\" value=\"addToGlobalBlockUrl\" onClick=\"action('addToGlobalBlockUrl','" + url + "');\">" : "";
							ba.append( QString("<td>" + url + "</td><td>" + adminB + "</td></td>") );
							ba.append("</tr>");
						}
						ba.append("</table>");
					}
					if( type.first == "con" && value == "openAddrs" ){
						ba.append("openAddrs:>:");
						ba.append("<table>");
						for( auto addr:app::state.addrs ){
							ba.append("<tr>");
							QString adminB = ( userData.group == UserGrpup::admins ) ? "<input type=\"button\" value=\"addToGlobalBlockAddr\" onClick=\"action('addToGlobalBlockAddr','" + addr + "');\">" : "";
							ba.append( QString("<td>" + addr + "</td><td>" + adminB + "</td></td>") );
							ba.append("</tr>");
						}
						ba.append("</table>");
					}
					if( type.first == "con" && value == "users" ){
						ba.append("users:>:");
						QDateTime dt = QDateTime::currentDateTime();
						ba.append("<table>");
						for( auto user:app::conf.users ){
							uint32_t lastLoginSec = dt.toTime_t() - user.lastLoginTimestamp;
							ba.append("<tr>");
							ba.append( QString("<td>" + user.login + "</td><td> " + QString::number( lastLoginSec ) + " sec. ago</td>") );
							ba.append( QString("<td> " + QString::number( user.connections ) + " / " + QString::number( user.maxConnections ) + "</td>") );
							ba.append("</tr>");
						}
						ba.append("</table>");
					}
				}
			}
		}

		if( method == "set" ){
			ba.append("OK");
			if( args.count("param") > 0 && args.count("value") ){
				auto param = args.at("param")[0];
				auto value = args.at("value");
				if( param == "addToGlobalBlockUrl" && userData.group == UserGrpup::admins ){
					for( auto elem:value ) app::addGlobalBlackUrl( elem );
				}
				if( param == "addToGlobalBlockAddr" && userData.group == UserGrpup::admins ){
					for( auto elem:value ) app::addGlobalBlackAddr( elem );
				}
			}
		}

		return ba;
	}

	bool findInBlackList(const QString &url, const std::vector<QString> &data)
	{
		bool res = false;

		for( auto elem:data ){
			if( mf::strFind( elem, url ) ){
				res = true;
				break;
			}
		}

		return res;
	}

	QString getAuthString()
	{
		QString str;

		switch (app::conf.authMethod){
			case  http::AuthMethod::Basic:	str += "Basic realm=\"" + app::conf.realmString + "\"";		break;
			case  http::AuthMethod::Digest:
				//str += "Digest realm=\"" + app::conf.realmString + "\", qop=\"auth,auth-int\", nonce=\"" + mf::md5(dtStr).toHex() + "\",opaque=\"" + mf::md5(app::conf.realmString).toHex() + "\"";
				str += "Digest realm=\"" + app::conf.realmString + "\", qop=auth, nonce=\"" + app::getNonceCode() + "\",opaque=\"" + mf::md5(app::conf.realmString).toHex() + "\"";
				//str += "Digest realm=\"" + app::conf.realmString + "\", nonce=\"" + app::getNonceCode() + "\",opaque=\"" + mf::md5(app::conf.realmString).toHex() + "\"";
			break;
		}
		return str;
	}

	QByteArray getNonceCode()
	{
		QDateTime dt = QDateTime::currentDateTime();
		QByteArray dtStr;
		dtStr.append( dt.toString("yyyy.MM.dd") );
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

	void updateLoginTimestamp(const QString &login)
	{
		for( auto &user:app::conf.users ){
			if( login == user.login ){
				QDateTime dt = QDateTime::currentDateTime();
				user.lastLoginTimestamp = dt.toTime_t();
				break;
			}
		}
	}

}
