#include "global.h"

#include <QDateTime>
#include <QSettings>
#include <QHostInfo>

namespace app {
	Config conf;
	BlackList blackList;

	void loadSettings()
	{
		QSettings settings("MySoft","MyProxy");

		app::conf.port = settings.value("SOCKS PROXY/port",app::conf.port).toUInt();
		app::conf.blackAddrsFile = settings.value("SOCKS PROXY/blackAddrsFile",app::conf.blackAddrsFile).toString();
		app::conf.logFile = settings.value("SOCKS PROXY/logFile",app::conf.logFile).toString();
		app::conf.logLevel = settings.value("SOCKS PROXY/logLevel",app::conf.logLevel).toUInt();


		app::loadBlackList( app::conf.blackAddrsFile, app::blackList.nameAddrs );

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
			settings.setValue("SOCKS PROXY/port",app::conf.port);
			settings.setValue("SOCKS PROXY/blackAddrsFile",app::conf.blackAddrsFile);
			settings.setValue("SOCKS PROXY/logFile",app::conf.logFile);
			settings.setValue("SOCKS PROXY/logLevel",app::conf.logLevel);

			app::conf.saveSettings = false;
		}
		if( app::blackList.addrsFileSave ){
			app::saveBlackList( app::conf.blackAddrsFile, app::blackList.nameAddrs );
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
		app::setLog( 3, "SAVE BLACK ADDRS LIST..." );
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
		app::setLog( 4, "SAVE BLACK ADDRS LIST..." );
		bool find = false;

		for( auto elem:app::blackList.nameAddrs ){
			if( elem == str ){
				find = true;
				break;
			}
		}

		if( !find ){
			app::blackList.nameAddrs.push_back( str );
			app::blackList.addrsFileSave = true;
		}
	}

	void updateBlackIPAddrs()
	{
		app::setLog( 3, "UPDATE BLACK IP LIST..." );

		app::blackList.ipAddrs.clear();

		for( auto addr:app::blackList.nameAddrs ){
			auto info = QHostInfo::fromName( addr );
			app::setLog( 3, QString("GET INFO for [%1]").arg(addr) );
			if( info.error() == QHostInfo::NoError ){
				for(auto elem:info.addresses()){
					app::blackList.ipAddrs.push_back( elem );
					app::setLog( 3, QString("SET IP [%1]").arg(elem.toString()) );
				}
			}
		}
	}

	bool findBlockAddr(const QHostAddress& addr)
	{
		bool res = false;

		for( auto elem:app::blackList.ipAddrs ){
			if( elem == addr ){
				res = true;
				break;
			}
		}

		return res;
	}

}
