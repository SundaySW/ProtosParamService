//
// Created by outlaw on 28.10.2022.
//

#ifndef POTOSSERVER_PARAMSERVICE_PARAMSERVICE_H
#define POTOSSERVER_PARAMSERVICE_PARAMSERVICE_H

#include <QtCore/QTimer>
#include <Monitor/protos_message.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <socket_adapter.h>
#include "ParamItem.h"
#include "QJsonObject"
#include "QJsonArray"
#include "QJsonDocument"
#include "QFile"

class ParamService: public QObject{
    Q_OBJECT
public:
    explicit ParamService(QJsonObject &inJson);
    ParamService(const ParamService&) = delete;

    bool addParam(ParamItem &&paramItem, bool addToDB=true);
    bool addParam(uchar, uchar, ParamItemType);
    bool removeParam(const QString&);
    bool updateParam(const ProtosMessage &message, const QString &mapKey);
    void configureTimer(int);
    void saveParams();
    void loadParams();
    bool isSocketConnected();
    SocketAdapter& getSocketAdapter();
    void setParamValueChanged(const QString&, const QVariant& value);
    static QString makeMapKey(uchar host, uchar ID);
    static QString makeMapKey(const ParamItem &paramItem);
    void setSelfAddr(uchar selfAddr);
    void setWriteToFile(bool writeToFile);
    bool isWriteToFile() const;
    uchar getSelfAddr() const;
    void closeAll();
    void configParam(const QString &mapKey);
    void setReqOnSet(bool _reqOnSet);
    bool isReqOnSet() const;
    QSet<uchar> getAllHostsInSet();
    QSet<QString> getAllParams();
    void removeAllParams();
signals:
    void changedParamState(ParamItemType);
    void changedParamValue(const ParamItem& param);
    void addedParamFromLine(ParamItemType);
    void addedParamFromLine(const ParamItem& param);
    void needToResetModels();
private slots:
    void writeTimerUpdate();
private:
    uchar selfAddr;
    QMap<QString, ParamItem> paramsMap;
    QMap<QString, QTimer*> timerMap;

    QTimer* writeTimer;
    int mSecWriteTimer = 1000;
    bool writeToFile;
    bool reqOnSet;
    QFile* logFile;
    QTextStream textStream;
    QJsonObject& qJsonObject;
    SocketAdapter socketAdapter;
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
    void updateParamUpdateRate(const QString &mapKey, const QVariant &value);
    void processPSETMsg(const ProtosMessage &message);
};
#endif //POTOSSERVER_PARAMSERVICE_PARAMSERVICE_H