//
// Created by outlaw on 27.10.2022.
//

#ifndef POTOSSERVER_PARAMSERVICE_PARAMITEM_H
#define POTOSSERVER_PARAMSERVICE_PARAMITEM_H

#include <QtCore/QtGlobal>
#include <QtCore/QVariant>
#include <Monitor/protos_message.h>
#include <QtCore/QTimer>
#include "QDateTime"
#include "QJsonObject"

enum ParamItemType {
    BASE = 0,
    UPDATE = 1,
    CONTROL = 2,
    CALIB = 3
};

enum ParamItemStates {
    ONLINE = 0,
    OFFLINE = 1,
    PENDING = 2,
    DB_WRITE_FAIL = 3,
    ERROR = 4,
};

class ParamItem: public QObject{
    Q_OBJECT
public:
    ParamItem() = delete;
    explicit ParamItem(uchar, uchar, ParamItemType);
    explicit ParamItem(QJsonObject&);
    explicit ParamItem(const ProtosMessage &message, ParamItemType type);
    explicit ParamItem(ParamItem&& paramItem);

    [[nodiscard]] uchar getParamId() const;
    [[nodiscard]] uchar getHostID() const;
    [[nodiscard]] const QVariant &getValue() const;
    [[nodiscard]] const QString &getNote() const;
    [[nodiscard]] ParamItemStates getState() const;
    [[nodiscard]] ParamItemType getParamType() const;
    [[nodiscard]] bool isWriteToDb() const;
    [[nodiscard]] QString getTableName() const;
    [[nodiscard]] int getViewUpdateRate() const;
    int getUpdateRate() const;
    QString getLastValueTime();
    QString getLastValueDay();
    const QDateTime &getLastValueDateTime() const;

    QJsonObject getJsonObject();
    static QString getColumnTypes();
    static QString getTableInsertVars();
    QString getLogToFileStr(const QString &eventStr);
    QString getTableInsertValues(const QString &eventStr = "") const;
    QString getAltName() const;
    uchar getSenderId() const;
    const QVariant &getExpectedValue() const;
    ProtosMessage::MsgTypes getLastValueType() const;
    uchar getDestId() const;
    short getRateValue(uchar rateType) const;
    double getCalibValue(uchar rateType) const;

    void setUpdateRate(short value);
    void setViewUpdateRate(short updateRate);
    void setValue(const QVariant &value);
    void setLastValueType(ProtosMessage::MsgTypes lastValueType);
    void setExpectedValue(const QVariant &value);
    void setNote(const QString &note);
    void setState(ParamItemStates inState);
    void setWriteToDb(bool writeToDb);
    void setAltName(const QString&);
    void setLastValueTime(QDateTime lastValueTime);
    bool setParamType(ParamItemType paramType);
    void setSenderId(uchar senderId);
    void setDestId(uchar destID);
    void setRateValue(uchar rateType, short Value);
    void setCalibValue(uchar rateType, double Value);

    void timeoutUpdate();
    void update(const ProtosMessage &message);
    static QString getLogToFileHead();
signals:
    void newParamValue(const QVariant &value);
    void paramRatesChanged(uchar, short);
    void paramCalibDataChanged(uchar, double);
private:
    QDateTime lastValueTime;
    uchar ID;
    uchar Host_ID;
    uchar Sender_ID = 0;
    uchar Dest_ID = 0;
    QString altName;
    QVariant Value;
    QVariant expectedValue;
    ProtosMessage::MsgTypes lastValueType;
    QString Note;
    ParamItemStates state;
    ParamItemType paramType;
    bool writeToDB;
    int viewUpdateRate = 5000;
    short paramUpdateRate, paramSendRate, paramCtrlRate;
    double paramCalibOffset, paramCalibMult;
};

#endif //POTOSSERVER_PARAMSERVICE_PARAMITEM_H