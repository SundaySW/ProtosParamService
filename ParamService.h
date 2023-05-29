//
// Created by outlaw on 28.10.2022.
//

#ifndef POTOSSERVER_PARAMSERVICE_PARAMSERVICE_H
#define POTOSSERVER_PARAMSERVICE_PARAMSERVICE_H

#include <QtCore/QTimer>
#include <Monitor/protos_message.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <Monitor/socket_adapter.h>
#include "Param_Item/ParamItem.h"
#include "QJsonObject"
#include "QJsonArray"
#include "QJsonDocument"
#include "QFile"

class ParamService: public QObject{
    Q_OBJECT
public:
    explicit ParamService();
    ParamService(const ParamService&) = delete;
    ParamService(ParamService&) = delete;
    ParamService(ParamService*) = delete;

    void setSocketAdapter(QSharedPointer<SocketAdapter> SA);
    bool addParam(ParamItem &&paramItem, bool addToDB=true);
    bool addParam(uchar, uchar, ParamItemType);
    bool removeParam(const QString&);
    bool updateParam(const ProtosMessage &message, const QString &mapKey);
    void configureTimer(int);
    void saveParams(QJsonObject& qJsonObject);
    void loadParams(QJsonObject& qJsonObject);
    void setParamValueChanged(const QString&, const QVariant& value);
    void setParamValueChanged(uchar Id, uchar Host, const QVariant &value);
    static QString makeMapKey(uchar host, uchar ID);
    static QString makeMapKey(const ParamItem &paramItem);
    static QString makeMapKey(const ParamItem *paramItem);
    static QString makeMapKey(const QJsonObject &jsonObject);

    static QString tableNameToMapKey(QString tableName);
    void setSelfAddr(uchar selfAddr);
    void setWriteToFile(bool writeToFile);
    bool isWriteToFile() const;
    uchar getSelfAddr() const;
    void closeAll();
    void configParam(const QString &mapKey);
    void setReqOnSet(bool _reqOnSet);
    bool isReqOnSet() const;
    QSet<uchar> getAllHostsInSet();
    QStringList getAllParamsStrList();
    void removeAllParams();
    QSharedPointer<ParamItem> getParam(const QString&);
    bool containsParam(const QString&);
    template<typename T>
    void sendServiceMsgSet(ParamItem *paramItem, const T &value, ProtosMessage::ParamFields field);
    void sendServiceMsgReq(ParamItem *paramItem, ProtosMessage::ParamFields field);
signals:
    void changedParamState(ParamItemType);
    void changedParamValue(const QSharedPointer<ParamItem> param);
    void addedParamFromLine(ParamItemType);
    void addedParamFromLine(const QSharedPointer<ParamItem> param);
    void needToResetModels();
private slots:
    void writeTimerUpdate();
private:
    uchar selfAddr;
    QMap<QString, QSharedPointer<ParamItem>> paramsMap;
    QMap<QString, QTimer*> timerMap;

    QTimer* writeTimer;
    int mSecWriteTimer = 1000;
    bool writeToFile;
    bool reqOnSet;
    QFile* logFile;
    QTextStream textStream;
    QSharedPointer<SocketAdapter> socketAdapter;
    void txMsgHandler(const ProtosMessage &message);
    void rxMsgHandler(const ProtosMessage &message);
    void timerFinished(int timID);
    void makeParamTimer(const QString &mapKey);
    void writeParamToFile(ParamItem &param, const QString &event);
    void setParamEvent(const QString &mapKey);
    void sendProtosMsgSetParam(const QString &mapKey);
    void processSetParamReq(const QString &mapKey);
    void manageTimersWhileUpdate(const QString &mapKey, uchar msgType, int updateRate, int paramType);
    void processPANSMsg(const ProtosMessage &message);
    void removeFromAllMaps(const QString &mapKey);
    void updateParamViewUpdateRate(const QString &mapKey, const QVariant &value);
    void processPSETMsg(const ProtosMessage &message);
    void updateParamUpdateRates(const QString &mapKey, const QVariant &value, uchar paramField);
    void updateParamCalibData(const QString &mapKey, const QVariant &value, uchar paramField);

    template<typename T = short>
    void makeParamServiceMsg(ParamItem* paramItem, ProtosMessage::ParamFields field, T value = 0, bool set = false);
};
#endif //POTOSSERVER_PARAMSERVICE_PARAMSERVICE_H