//
// Created by outlaw on 27.10.2022.
//

#include "ParamItem.h"

ParamItem::ParamItem(uchar incomeID, uchar hostAddr, ParamItemType type)
    :ID(incomeID),
    Host_ID(hostAddr),
    altName(""),
    Value(""),
    Note(("")),
    paramType(type),
    expectedValue(""),
    lastValueType(type == CONTROL ? ProtosMessage::MsgTypes::PSET : ProtosMessage::MsgTypes::PANS),
    writeToDB(false),
    state(OFFLINE)
{}

ParamItem::ParamItem(const ProtosMessage &message, ParamItemType type){
    ID = message.GetParamId();
    if(type == CONTROL)
        Host_ID = message.GetDestAddr();
    else Host_ID = message.GetSenderAddr();
    Dest_ID = message.GetDestAddr();
    Sender_ID = message.GetSenderAddr();
    altName = "";
    setValue(message.GetParamFieldValue());
    expectedValue = message.GetParamFieldValue();
    Note = "";
//    writeToDB = (type == CONTROL);
    writeToDB = false;
    paramType = UPDATE; //todo check about it
    state = ONLINE;
    lastValueTime = QDateTime::currentDateTime();
    lastValueType = (ProtosMessage::MsgTypes)message.MsgType;
}

ParamItem::ParamItem(QJsonObject& obj){
    ID = obj["ID"].toInt();
    Host_ID = obj["HostID"].toInt();
    Dest_ID = obj["DestID"].toInt();
    Sender_ID = obj["SenderID"].toInt();
    altName = obj["altName"].toString();
    setValue(obj["Value"].toString());
    expectedValue = obj["ExpectedValue"].toString();
    Note = obj["Note"].toString();
    writeToDB = obj["writeToDB"].toBool();
    paramType = static_cast<ParamItemType>(obj["Type"].toInt());
    state = ParamItemStates(obj["state"].toInt());
    lastValueTime = QDateTime::fromString(obj["dateTime"].toString());
    lastValueType = static_cast<ProtosMessage::MsgTypes>(obj["LastValueType"].toInt());
    viewUpdateRate = obj["UpdateRate"].toInt();
}

ParamItem::ParamItem(ParamItem &&paramItem) {
    ID = paramItem.ID;
    Host_ID = paramItem.Host_ID;
    Dest_ID = paramItem.Dest_ID;
    Sender_ID = paramItem.Sender_ID;
    altName = paramItem.altName;
    setValue(paramItem.Value);
    expectedValue = paramItem.expectedValue;
    Note = paramItem.Note;
    writeToDB = paramItem.writeToDB;
    paramType = paramItem.paramType;
    state = paramItem.state;
    lastValueTime = paramItem.lastValueTime;
    lastValueType = paramItem.lastValueType;
    viewUpdateRate = paramItem.viewUpdateRate;
}


QJsonObject ParamItem::getJsonObject() {
    QJsonObject retVal;
    retVal["ID"] = ID;
    retVal["HostID"] = Host_ID;
    retVal["DestID"] = Dest_ID;
    retVal["SenderID"] = Sender_ID;
    retVal["altName"] = altName;
    retVal["Value"] = Value.toString();
    retVal["ExpectedValue"] = expectedValue.toString();
    retVal["Note"] = Note;
    retVal["writeToDB"] = writeToDB;
    retVal["Type"] = paramType;
    retVal["state"] = static_cast<int>(state);
    retVal["dateTime"] = lastValueTime.toString();
    retVal["LastValueType"] = lastValueType;
    retVal["UpdateRate"] = viewUpdateRate;
    return retVal;
}

void ParamItem::update(const ProtosMessage &message){
    setLastValueType((ProtosMessage::MsgTypes)message.MsgType);
    setSenderId(message.GetSenderAddr());
    setDestId(message.GetDestAddr());
    setLastValueTime(QDateTime::currentDateTime());
    auto receivedValue = message.GetParamFieldValue();
    auto msgType = message.MsgType;
    if(lastValueType == ProtosMessage::PSET){
        expectedValue = receivedValue;
        setState(PENDING);
    }
    else if(lastValueType == ProtosMessage::PANS && paramType == CONTROL){
        if(receivedValue == expectedValue)
            setState(ONLINE);
    }
    if(msgType == ProtosMessage::PANS)
        setState(ONLINE);
    else if(msgType == ProtosMessage::PSET)
        setState(PENDING);
    setValue(message.GetParamFieldValue());
}

QString ParamItem::getColumnTypes() {
    return {
    "n SERIAL PRIMARY KEY,"
    " DateTime timestamp with time zone,"
    " Value real,"
    " MsgType smallint,"
    " SenderDest smallint,"
    " Note VARCHAR (70)"
    };
}

QString ParamItem::getTableInsertVars() {
    return {"DateTime, Value, MsgType, SenderDest, Note"};
}

QString ParamItem::getTableInsertValues(const QString &eventStr) const {
    return QString("current_timestamp, '%1', '%2', '%3', '%4'")
        .arg(Value.toString())
        .arg(lastValueType)
        .arg((lastValueType == ProtosMessage::PSET) ? Sender_ID : Dest_ID)
        .arg(Note);
}

QString ParamItem::getLogToFileHead() {
    return "Time_Stamp,Host_ID,Param_id,Value,Type,Sender/Dest_ID,Note\n";
}

QString ParamItem::getLogToFileStr(const QString &eventStr) {
    return QString("%1,%2,%3,%4,%5,%6,%7\n")
        .arg(QDateTime::currentDateTime().toString(QString("yyyy.MM.dd_hh.mm.ss")))
        .arg(getHostID(),0,16)
        .arg(getParamId(), 0, 16)
        .arg(Value.toString())
        .arg(ProtosMessage::GetMsgTypeName(lastValueType))
        .arg(lastValueType==ProtosMessage::PANS? Dest_ID : Sender_ID)
        .arg(Note);
}

QString ParamItem::getTableName() const{
    QString id_part = QString("%1").arg(ID,0, 16).prepend("0x");
    QString host_part = QString("%1").arg(Host_ID,0,16).prepend("0x");
    return QString("param_%1_host_%2").arg(id_part, host_part);
}

uchar ParamItem::getParamId() const {
    return ID;
}

uchar ParamItem::getHostID() const {
    return Host_ID;
}

const QVariant &ParamItem::getValue() const {
    return Value;
}

const QString &ParamItem::getNote() const {
    return Note;
}

ParamItemType ParamItem::getParamType() const {
    return paramType;
}

ParamItemStates ParamItem::getState() const {
    return state;
}

bool ParamItem::isWriteToDb() const {
    return writeToDB;
}

void ParamItem::setValue(const QVariant &value){
    Value = value;
    this->setLastValueTime(QDateTime::currentDateTime());
    emit newParamValue(value);
}

void ParamItem::setExpectedValue(const QVariant &value) {
    expectedValue = value;
}

void ParamItem::setNote(const QString &note) {
    Note = note;
}

void ParamItem::setState(ParamItemStates inState) {
    state = inState;
}

void ParamItem::setWriteToDb(bool writeToDb) {
    writeToDB = writeToDb;
}

QString ParamItem::getLastValueTime(){
    if(lastValueTime.isNull())
        return {"-- --"};
    return lastValueTime.toString(QString("hh:mm:ss_zzz"));
}

QString ParamItem::getLastValueDay() {
    if(lastValueTime.isNull())
        return {"-- --"};
    return lastValueTime.toString(QString("dd.MM"));
}

void ParamItem::setLastValueTime(QDateTime valueTime) {
    lastValueTime = std::move(valueTime);
}

void ParamItem::setAltName(const QString& name) {
    altName = name;
}

QString ParamItem::getAltName() const {
    return altName;
}

int ParamItem::getViewUpdateRate() const {
    return viewUpdateRate;
}

void ParamItem::timeoutUpdate(){
    if(paramType == UPDATE) setState(OFFLINE);
}

bool ParamItem::setParamType(ParamItemType inParamType) {
    if(paramType == inParamType) return false;
    else paramType = inParamType;
    writeToDB = true;
    return true;
}

uchar ParamItem::getSenderId() const {
    return Sender_ID;
}

void ParamItem::setSenderId(uchar senderId) {
    Sender_ID = senderId;
}

void ParamItem::setDestId(uchar destID) {
    Dest_ID = destID;
}

const QVariant &ParamItem::getExpectedValue() const {
    return expectedValue;
}

void ParamItem::setLastValueType(ProtosMessage::MsgTypes type) {
    lastValueType = type;
}

ProtosMessage::MsgTypes ParamItem::getLastValueType() const {
    return lastValueType;
}

uchar ParamItem::getDestId() const {
    return Dest_ID;
}

void ParamItem::setViewUpdateRate(short _updateRate) {
    viewUpdateRate = _updateRate;
}

short ParamItem::getRateValue(uchar paramField) const{
    switch (paramField) {
        case ProtosMessage::UPDATE_RATE:
            return paramUpdateRate;
        case ProtosMessage::SEND_RATE:
            return paramSendRate;
        case ProtosMessage::CTRL_RATE:
            return paramCtrlRate;
        default:
            return 0;
    }
}

double ParamItem::getCalibValue(uchar paramField) const {
    switch (paramField) {
        case ProtosMessage::MULT:
            return paramCalibMult;
        case ProtosMessage::OFFSET:
            return paramCalibOffset;
        default:
            return 0;
    }
}

void ParamItem::setRateValue(uchar paramField, short value){
    switch (paramField) {
        case ProtosMessage::UPDATE_RATE:
            paramUpdateRate = value;
            break;
        case ProtosMessage::SEND_RATE:
            paramSendRate = value;
        case ProtosMessage::CTRL_RATE:
            paramCtrlRate = value;
        default:
            break;
    }
    emit paramRatesChanged(paramField, value);
}

void ParamItem::setCalibValue(uchar paramField, double value){
    switch (paramField) {
        case ProtosMessage::MULT:
            paramCalibMult = value;
        case ProtosMessage::OFFSET:
            paramCalibOffset = value;
        default:
            break;
    }
    emit paramCalibDataChanged(paramField, value);
}

int ParamItem::getUpdateRate() const {
    return paramUpdateRate;
}

const QDateTime &ParamItem::getLastValueDateTime() const {
    return lastValueTime;
}
