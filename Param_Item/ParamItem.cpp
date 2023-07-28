//
// Created by outlaw on 27.10.2022.
//

#include "ParamItem.h"

ParamItem::ParamItem(uchar incomeID, uchar hostAddr, ParamItemType type)
    : ID_(incomeID),
      host_id_(hostAddr),
      alt_name_(""),
      value_(""),
      note_(("")),
      param_type_(type),
      expected_value_(""),
      last_value_type_(type == ParamItemType::kControl ? ProtosMessage::MsgTypes::PSET : ProtosMessage::MsgTypes::PANS),
      write_to_DB_(false),
      state_(ParamItemStates::kOffline)
{}

ParamItem::ParamItem(const ProtosMessage &message, ParamItemType type){
    ID_ = message.GetParamId();
    if(type == ParamItemType::kControl)
        host_id_ = message.GetDestAddr();
    else host_id_ = message.GetSenderAddr();
    dest_id_ = message.GetDestAddr();
    sender_id_ = message.GetSenderAddr();
    alt_name_ = "";
    setValue(message.GetParamFieldValue());
    expected_value_ = message.GetParamFieldValue();
    note_ = "";
//    write_to_DB_ = (type == kControl);
    write_to_DB_ = false;
    param_type_ = ParamItemType::kUpdate; //todo check about it
    state_ = ParamItemStates::kOnline;
    last_value_time_ = QDateTime::currentDateTime();
    last_value_type_ = (ProtosMessage::MsgTypes)message.MsgType;
}

ParamItem::ParamItem(QJsonObject& obj){
    ID_ = obj["ID_"].toInt();
    host_id_ = obj["HostID"].toInt();
    dest_id_ = obj["DestID"].toInt();
    sender_id_ = obj["SenderID"].toInt();
    alt_name_ = obj["alt_name_"].toString();
    setValue(obj["value_"].toString());
    expected_value_ = obj["ExpectedValue"].toString();
    note_ = obj["note_"].toString();
    write_to_DB_ = obj["write_to_DB_"].toBool();
    param_type_ = static_cast<ParamItemType>(obj["Type"].toInt());
//    state_ = ParamItemStates(obj["state_"].toInt());
    state_ = ParamItemStates::kOffline;
    last_value_time_ = QDateTime::fromString(obj["dateTime"].toString());
    last_value_type_ = static_cast<ProtosMessage::MsgTypes>(obj["LastValueType"].toInt());
    param_update_rate_ = (short)obj["UpdateRate"].toInt();
    view_update_rate_ = obj["ViewUpdateRate"].toInt();
}

ParamItem::ParamItem(ParamItem &&paramItem) {
    ID_ = paramItem.ID_;
    host_id_ = paramItem.host_id_;
    dest_id_ = paramItem.dest_id_;
    sender_id_ = paramItem.sender_id_;
    alt_name_ = paramItem.alt_name_;
    setValue(paramItem.value_);
    expected_value_ = paramItem.expected_value_;
    note_ = paramItem.note_;
    write_to_DB_ = paramItem.write_to_DB_;
    param_type_ = paramItem.param_type_;
    state_ = paramItem.state_;
    last_value_time_ = paramItem.last_value_time_;
    last_value_type_ = paramItem.last_value_type_;
    view_update_rate_ = paramItem.view_update_rate_;
}

QJsonObject ParamItem::getJsonObject() {
    QJsonObject retVal;
    retVal["ID_"] = ID_;
    retVal["HostID"] = host_id_;
    retVal["DestID"] = dest_id_;
    retVal["SenderID"] = sender_id_;
    retVal["alt_name_"] = alt_name_;
    retVal["value_"] = value_.toString();
    retVal["ExpectedValue"] = expected_value_.toString();
    retVal["note_"] = note_;
    retVal["write_to_DB_"] = write_to_DB_;
    retVal["Type"] = (int)param_type_;
    retVal["state_"] = static_cast<int>(state_);
    retVal["dateTime"] = last_value_time_.toString();
    retVal["LastValueType"] = last_value_type_;
    retVal["UpdateRate"] = param_update_rate_;
    retVal["ViewUpdateRate"] = view_update_rate_;
    return retVal;
}

void ParamItem::update(const ProtosMessage &message){
    setLastValueType((ProtosMessage::MsgTypes)message.MsgType);
    setSenderId(message.GetSenderAddr());
    setDestId(message.GetDestAddr());
    setLastValueTime(QDateTime::currentDateTime());
    auto receivedValue = message.GetParamFieldValue();
    auto msgType = message.MsgType;
    if(last_value_type_ == ProtosMessage::PSET){
        expected_value_ = receivedValue;
        setState(ParamItemStates::kPending);
    }
    else if(last_value_type_ == ProtosMessage::PANS && param_type_ == ParamItemType::kControl){
        if(receivedValue == expected_value_)
            setState(ParamItemStates::kOnline);
    }
    if(msgType == ProtosMessage::PANS)
        setState(ParamItemStates::kOnline);
    else if(msgType == ProtosMessage::PSET)
        setState(ParamItemStates::kPending);
    setValue(message.GetParamFieldValue());
}

QString ParamItem::getColumnTypes() {
    return {
    "n SERIAL PRIMARY KEY,"
    " DateTime timestamp with time zone,"
    " value_ real,"
    " MsgType smallint,"
    " SenderDest smallint,"
    " note_ VARCHAR (70)"
    };
}

QString ParamItem::getTableInsertVars() {
    return {"DateTime, value_, MsgType, SenderDest, note_"};
}

QString ParamItem::getTableInsertValues(const QString &eventStr) const {
    return QString("current_timestamp, '%1', '%2', '%3', '%4'")
        .arg(value_.toString())
        .arg(last_value_type_)
        .arg((last_value_type_ == ProtosMessage::PSET) ? sender_id_ : dest_id_)
        .arg(note_);
}

QString ParamItem::getLogToFileHead() {
    return "Time_Stamp,host_id_,Param_id,value_,Type,Sender/dest_id_,note_\n";
}

QString ParamItem::getLogToFileStr(const QString &eventStr) {
    return QString("%1,%2,%3,%4,%5,%6,%7\n")
        .arg(QDateTime::currentDateTime().toString(QString("yyyy.MM.dd_hh.mm.ss")))
        .arg(getHostID(),0,16)
        .arg(getParamId(), 0, 16)
        .arg(value_.toString())
        .arg(ProtosMessage::GetMsgTypeName(last_value_type_))
        .arg(last_value_type_ == ProtosMessage::PANS ? dest_id_ : sender_id_)
        .arg(note_);
}

QString ParamItem::getTableName() const{
    QString id_part = QString("%1").arg(ID_, 0, 16).prepend("0x");
    QString host_part = QString("%1").arg(host_id_, 0, 16).prepend("0x");
    return QString("param_%1_host_%2").arg(id_part, host_part);
}

uchar ParamItem::getParamId() const {
    return ID_;
}

uchar ParamItem::getHostID() const {
    return host_id_;
}

const QVariant &ParamItem::getValue() const {
    return value_;
}

const QString &ParamItem::getNote() const {
    return note_;
}

ParamItemType ParamItem::getParamType() const {
    return param_type_;
}

ParamItemStates ParamItem::getState() const {
    return state_;
}

bool ParamItem::isWriteToDb() const {
    return write_to_DB_;
}

void ParamItem::setValue(const QVariant &value){
    value_ = value;
    this->setLastValueTime(QDateTime::currentDateTime());
    emit newParamValue(value);
}

void ParamItem::setExpectedValue(const QVariant &value) {
    expected_value_ = value;
}

void ParamItem::setNote(const QString &note) {
    note_ = note;
}

void ParamItem::setState(ParamItemStates inState) {
    state_ = inState;
}

void ParamItem::setWriteToDb(bool writeToDb) {
    write_to_DB_ = writeToDb;
}

QString ParamItem::getLastValueTime(){
    if(last_value_time_.isNull())
        return {"-- --"};
    return last_value_time_.toString(QString("hh:mm:ss_zzz"));
}

QString ParamItem::getLastValueDay() {
    if(last_value_time_.isNull())
        return {"-- --"};
    return last_value_time_.toString(QString("dd.MM"));
}

void ParamItem::setLastValueTime(QDateTime valueTime) {
    last_value_time_ = std::move(valueTime);
}

void ParamItem::setAltName(const QString& name) {
    alt_name_ = name;
}

QString ParamItem::getAltName() const {
    return alt_name_;
}

int ParamItem::getViewUpdateRate() const {
    return view_update_rate_;
}

void ParamItem::timeoutUpdate(){
    if(param_type_ == ParamItemType::kUpdate) setState(ParamItemStates::kOffline);
}

bool ParamItem::setParamType(ParamItemType inParamType) {
    if(param_type_ == inParamType) return false;
    else param_type_ = inParamType;
    write_to_DB_ = true;
    return true;
}

uchar ParamItem::getSenderId() const {
    return sender_id_;
}

void ParamItem::setSenderId(uchar senderId) {
    sender_id_ = senderId;
}

void ParamItem::setDestId(uchar destID) {
    dest_id_ = destID;
}

const QVariant &ParamItem::getExpectedValue() const {
    return expected_value_;
}

void ParamItem::setLastValueType(ProtosMessage::MsgTypes type) {
    last_value_type_ = type;
}

ProtosMessage::MsgTypes ParamItem::getLastValueType() const {
    return last_value_type_;
}

uchar ParamItem::getDestId() const {
    return dest_id_;
}

void ParamItem::setViewUpdateRate(short _updateRate) {
    view_update_rate_ = _updateRate;
}

short ParamItem::getRateValue(uchar paramField) const{
    switch (paramField) {
        case ProtosMessage::UPDATE_RATE:
            return param_update_rate_;
        case ProtosMessage::SEND_RATE:
            return param_send_rate_;
        case ProtosMessage::CTRL_RATE:
            return param_ctrl_rate_;
        default:
            return 0;
    }
}

double ParamItem::getCalibValue(uchar paramField) const {
    switch (paramField) {
        case ProtosMessage::MULT:
            return param_calib_mult_;
        case ProtosMessage::OFFSET:
            return param_calib_offset_;
        default:
            return 0;
    }
}

void ParamItem::setRateValue(uchar paramField, short value){
    switch (paramField) {
        case ProtosMessage::UPDATE_RATE:
            param_update_rate_ = value;
            break;
        case ProtosMessage::SEND_RATE:
            param_send_rate_ = value;
        case ProtosMessage::CTRL_RATE:
            param_ctrl_rate_ = value;
        default:
            break;
    }
    emit paramRatesChanged(paramField, value);
}

void ParamItem::setCalibValue(uchar paramField, double value){
    switch (paramField) {
        case ProtosMessage::MULT:
            param_calib_mult_ = value;
        case ProtosMessage::OFFSET:
            param_calib_offset_ = value;
        default:
            break;
    }
    emit paramCalibDataChanged(paramField, value);
}

int ParamItem::getUpdateRate() const {
    return param_update_rate_;
}

void ParamItem::setUpdateRate(short value) {
    param_update_rate_ = value;
}

const QDateTime &ParamItem::getLastValueDateTime() const {
    return last_value_time_;
}
