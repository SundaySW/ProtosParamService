//
// Created by AlexKDR on 23.10.2022.
//

#ifndef POTOSSERVER_PARAMSERVICE_PSQL_DRIVER_H
#define POTOSSERVER_PARAMSERVICE_PSQL_DRIVER_H

#include <QSqlDatabase>
#include <Param_Item/ParamItem.h>

class PSQL_Driver {
public:
    std::function<void(const QString&)> errorInDBDriver;
    std::function<void(const QString&)> eventInDBDriver;
    explicit PSQL_Driver(QJsonObject &conf);
    bool setConnection();
    bool isAutoConnect() const;
    void closeConnection();
    bool writeParam(const ParamItem &param, const QString &event = "");
    void writeEvent(const QString &eventStr);
    void writeViewChanges(const QString &eventStr);
    [[nodiscard]] bool isDBOk() const;
private:
    bool hasTable(const QString &tableName);
    bool sendReq(const QString &queryStr);
    QSqlDatabase db;
    QJsonObject& config;
    QString host, dbName, userName, password, port;
    QString eventsTableName = "events";
    QString viewChangesTName = "view_changes";
    QSet<QString> tableNames;
    bool inError;
    bool connected;
    bool autoConnect;
    bool createParamTable(const QString &tableName);
    void throwEventToLog(const QString &str);
    void throwErrorToLog(const QString &str);
    bool createEventsTable(const QString &tableName);
    static QString getTableInsertVars();
    static QString getTableInsertValues(const QString &eventStr);
    void loadTableNames();
    void configUpdate();
    bool execMyQuery(QSqlQuery&, const QString&);
};


#endif //POTOSSERVER_PARAMSERVICE_PSQL_DRIVER_H
