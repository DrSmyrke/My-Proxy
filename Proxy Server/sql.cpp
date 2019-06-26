#include "sql.h"

#include <QSql>
#include <QSqlQuery>
#include <QSqlResult>
#include <QSqlError>
#include <QSqlRecord>

namespace db {
	QSqlDatabase sdb;

	void openBase(const QString &file, const uint8_t &baseType)
	{
		if( db::sdb.isOpen() ) db::sdb.close();

		bool correctType = false;
		switch( baseType ) {
			case DataBaseType::SQLITE:
				db::sdb = QSqlDatabase::addDatabase("QSQLITE");
				correctType = true;
			break;
			case DataBaseType::SQL:
				//db::sdb = QSqlDatabase::addDatabase("QSQL");
				//correctType = true;
			break;
		}
		if( correctType ){
			db::sdb.setDatabaseName( file );
			db::sdb.open();
		}
	}

	bool tableExsist(const QString &tableName)
	{
		QSqlQuery a_query;
		return a_query.exec( QString( "SELECT * FROM " + tableName ) );
	}

	bool createTable(const QString &tableName, const std::map<QString, QString> &fields)
	{
		if( fields.size() <= 0 ) return false;

		QSqlQuery a_query;
		QList<QString> fieldsData;

		for( auto elem:fields ) fieldsData.push_back( QString( "`" + elem.first + "`	" + elem.second ) );

		return a_query.exec( QString("CREATE TABLE `" + tableName + "` ( " + fieldsData.join( ", " ) + " );") );
	}

	QMap<QString, QVariant> getData(const QString &tableName)
	{
		QMap<QString, QVariant> data;

		QSqlQuery a_query;
		bool res = a_query.exec( QString( "SELECT * FROM " + tableName ) );
		if( res ) data = a_query.boundValues();

		return data;
	}

}
