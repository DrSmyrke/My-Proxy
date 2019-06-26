#ifndef SQL_H
#define SQL_H

#include <QSqlDatabase>
#include <QVariant>
#include <map>



namespace db {
	struct BaseType{
		enum{
			SQLITE,
			SQL,
		};
	};
	extern QSqlDatabase sdb;

	void openBase(const QString &file, const uint8_t &baseType);
	bool tableExsist(const QString &tableName);
	bool createTable(const QString &tableName, const std::map<QString, QString> &fields);
	QMap<QString, QVariant> getData(const QString &tableName);
}

#endif // SQL_H
