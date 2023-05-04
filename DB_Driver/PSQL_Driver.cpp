//
// Created by AlexKDR on 23.10.2022.
//

#include "PSQL_Driver.h"
#include "QSqlQuery"
#include "QSqlError"

PSQL_Driver::PSQL_Driver(QJsonObject& conf)
        :config(conf)
        ,inError(false)
        ,connected(false)
{
    configUpdate();
}

bool PSQL_Driver::setConnection() {
    bool result;
    db = QSqlDatabase::addDatabase("QPSQL");
    configUpdate();
    db.setHostName(host);
    db.setDatabaseName(dbName);
    db.setUserName(userName);
    db.setPassword(password);
    db.setPort(port.toInt());
    result = db.open();
    if(result){
        connected = true;
        inError = false;
        throwEventToLog(QString("opened database name: %1").arg(dbName));
        loadTableNames();
    }
    else {
        connected = false;
        inError = true;
        throwErrorToLog(QString("cant open database name: %1").arg(dbName));
    }
    return result;
}

void PSQL_Driver::loadTableNames(){
    QString queryStr = QString("SELECT table_name FROM information_schema.tables WHERE table_schema = 'public';");
    QSqlQuery query;
    if(!execMyQuery(query, queryStr)) {
//        qDebug() << query.lastError().text();
//        qDebug() << queryStr;
        throwErrorToLog("while check table: " + query.lastError().text());
        return;
    }
    QStringList results;
    tableNames.clear();
    while (query.next())
        tableNames.insert(query.value(0).toString());
}

bool PSQL_Driver::hasTable(const QString& tableName){
    return tableNames.contains(tableName);
/*    QString queryStr = QString(
            "SELECT EXISTS (SELECT FROM information_schema.tables "
            "WHERE table_schema LIKE 'public' "
            "AND table_type LIKE 'BASE TABLE' "
            "AND table_name = '%1');").arg(tableName);
    QSqlQuery query;
    if(!execMyQuery(query, queryStr)) {
//        qDebug() << query.lastError().text();
//        qDebug() << queryStr;
        throwErrorToLog("while check table: " + query.lastError().text());
    }
    bool retVal = false;
    if(query.next()) retVal = query.value(0).toBool();
    return retVal;*/
}

bool PSQL_Driver::sendReq(const QString &queryStr){
    if(inError) return false;
    QSqlQuery query;
    if(!query.exec(queryStr))
    {
        inError = true;
//        qDebug()<< "Query string: " << queryStr << " : " + query.lastError().text();
        throwErrorToLog("while sending query: " + query.lastError().text());
        return false;
    }else
        return true;
}

bool PSQL_Driver::execMyQuery(QSqlQuery& query, const QString& queryStr){
    if(inError) return false;
    bool result = query.exec(queryStr);
    if(!result) inError = true;
    return result;
}

bool PSQL_Driver::isDBOk() const {
    return connected && !inError;
}

bool PSQL_Driver::writeParam(const ParamItem &param, const QString &event){
    auto tableName = param.getTableName();
    if(!hasTable(tableName))
        if(createParamTable(tableName))
            throwEventToLog(QString("Table created: %1").arg(tableName));
    QString queryStr = QString("INSERT INTO %1 (%2) VALUES (%3);")
            .arg(tableName).arg(ParamItem::getTableInsertVars()).arg(param.getTableInsertValues(event));
    return sendReq(queryStr);
}

bool PSQL_Driver::createParamTable(const QString& tableName){
    QString queryStr = QString("CREATE TABLE IF NOT EXISTS %1 (%2);").arg(tableName).arg(ParamItem::getColumnTypes());
    bool retVal = sendReq(queryStr);
    if(retVal)
        tableNames.insert(tableName);
    return retVal;
}

bool PSQL_Driver::createEventsTable(const QString& tableName){
    QString columns(
            "n SERIAL PRIMARY KEY,"
            " DateTime TIMESTAMP,"
            " Event VARCHAR (100)"
    );
    QString queryStr = QString("CREATE TABLE IF NOT EXISTS %1 (%2);").arg(tableName).arg(columns);
    bool result = sendReq(queryStr);
    if(result)
        tableNames.insert(tableName);
    return result;
}

QString PSQL_Driver::getTableInsertVars() {
    return QString("DateTime, Event");
}

QString PSQL_Driver::getTableInsertValues(const QString &eventStr) {
    return QString("current_timestamp, '%1'").arg(eventStr);
}

void PSQL_Driver::writeEvent(const QString& eventStr){
    if(!hasTable(eventsTableName)){
        if(createEventsTable(eventsTableName))
            throwEventToLog(QString("Table created: %1").arg(eventsTableName));
    }
        QString queryStr = QString("INSERT INTO %1 (%2) VALUES (%3);")
                .arg(eventsTableName).arg(getTableInsertVars()).arg(getTableInsertValues(eventStr));
        sendReq(queryStr);
}

void PSQL_Driver::writeViewChanges(const QString& eventStr){
    if(!hasTable(viewChangesTName)){
        if(createEventsTable(viewChangesTName))
            throwEventToLog(QString("Table created: %1").arg(viewChangesTName));
    }
    QString queryStr = QString("INSERT INTO %1 (%2) VALUES (%3);")
            .arg(viewChangesTName).arg(getTableInsertVars()).arg(getTableInsertValues(eventStr));
    sendReq(queryStr);
}

void PSQL_Driver::configUpdate() {
    auto confObject = config.value("DBConfObject");
    host = confObject["HostName"].toString();
    dbName = confObject["DatabaseName"].toString();
    userName = confObject["UserName"].toString();
    password = confObject["Password"].toString();
    port = confObject["Port"].toString();
    autoConnect = confObject["autoconnect"].toBool();
}

void PSQL_Driver::closeConnection() {
    connected = false;
    db.close();
    throwEventToLog(QString("closed database name: %1").arg(dbName));
}

void PSQL_Driver::throwEventToLog(const QString& str){
    eventInDBDriver("Event in DB: " + str);
}

void PSQL_Driver::throwErrorToLog(const QString& str){
    errorInDBDriver("Error in DB: " + str);
}

bool PSQL_Driver::isAutoConnect() const {
    return autoConnect;
}
