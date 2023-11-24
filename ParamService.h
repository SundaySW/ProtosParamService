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
    bool addParam(ParamItem &&param_item, bool add_to_DB=true);
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
    QVariant getParamValue (const QString&);
    bool containsParam(const QString&);
    void sendServiceMsgReq(ParamItem *paramItem, ProtosMessage::ParamFields field);
    template<typename T>
        void sendServiceMsgSet(ParamItem *paramItem, const T &value, ProtosMessage::ParamFields field);
signals:
    void onChangedParamState(ParamItemType);
    void changedParamValue(const QSharedPointer<ParamItem> param);
    void addedParamFromLine(ParamItemType);
    void addedParamFromLine(const QSharedPointer<ParamItem> param);
    void needToResetModels();
private slots:
    void writeTimerUpdate();
private:
    uchar self_addr_;
    QMap<QString, QSharedPointer<ParamItem>> param_map_;
    QMap<QString, QTimer*> timer_map_;
    QTimer* write_timer_;
    const int kMSec_write_time_ = 1000;
    bool write_to_file_;
    bool request_on_set_;
    QFile* log_file_;
    QTextStream text_stream_;
    QSharedPointer<SocketAdapter> socket_adapter_;
    void txMsgHandler(const ProtosMessage &message);
    void rxMsgHandler(const ProtosMessage &message);
    void timerFinished(int timID);
    void makeParamTimer(const QString &mapKey);
    void writeParamToFile(ParamItem &param, const QString &event);
    void setParamEvent(const QString &mapKey);
    void sendProtosMsgSetParam(const QString &mapKey);
    void processSetParamReq(const QString &mapKey);
    void ManageTimersWhileUpdate(const QString &map_key, uchar msg_type, int update_rate, ParamItemType param_type);
    void processPANSMsg(const ProtosMessage &message);
    void removeFromAllMaps(const QString &mapKey);
    void updateParamViewUpdateRate(const QString &mapKey, const QVariant &value);
    void processSETMsg(const ProtosMessage &message);
    void updateParamUpdateRates(const QString &mapKey, const QVariant &value, uchar paramField);
    void updateParamCalibData(const QString &mapKey, const QVariant &value, uchar paramField);
    template<typename T = short>
        void makeParamServiceMsg(ParamItem* paramItem, ProtosMessage::ParamFields field, T value = 0, bool set = false);
};
#endif //POTOSSERVER_PARAMSERVICE_PARAMSERVICE_H