//
// Created by outlaw on 28.10.2022.
//

#include "ParamService.h"

#include <utility>

#ifdef _BUILD_TYPE_
#define CURRENT_BUILD_TYPE_ _BUILD_TYPE_
#else
#define CURRENT_BUILD_TYPE_ "CHECK CMAKE"
#endif

ParamService::ParamService():
        log_file_(new QFile()),
        write_to_file_(false)
{
}

void ParamService::setSocketAdapter(QSharedPointer<SocketAdapter> SA){
    socket_adapter_ = std::move(SA);
    socket_adapter_->AddTxMsgHandler([this](const ProtosMessage& txMsg) { txMsgHandler(txMsg);});
    socket_adapter_->AddRxMsgHandler([this](const ProtosMessage& txMsg) { rxMsgHandler(txMsg);});
}

bool ParamService::addParam(uchar incomeID, uchar incomeHostAddr, ParamItemType incomeType) {
    return addParam(std::move(ParamItem(incomeID, incomeHostAddr, incomeType)), false);
}

bool ParamService::addParam(ParamItem &&param_item, bool add_to_DB){
    auto map_key = makeMapKey(param_item);
    if(param_map_.contains(map_key)) return false;
    param_map_.insert(map_key, QSharedPointer<ParamItem>(new ParamItem(std::move(param_item))));
    auto appendParam = param_map_[map_key];
    auto type = appendParam->getParamType();
    makeParamTimer(map_key);
    emit addedParamFromLine(type);
    emit addedParamFromLine(param_map_[map_key]);
    return true;
}

bool ParamService::updateParam(const ProtosMessage &message, const QString &mapKey) {
    if(param_map_.contains(mapKey)){
        auto& paramToUpdate = param_map_[mapKey];
        paramToUpdate->update(message);
        emit onChangedParamState(paramToUpdate->getParamType());
        emit changedParamValue(paramToUpdate);
        ManageTimersWhileUpdate(mapKey, message.MsgType, paramToUpdate->getViewUpdateRate(),
                                paramToUpdate->getParamType());
        return true;
    }
    return false;
}

void ParamService::ManageTimersWhileUpdate(const QString &map_key, uchar msg_type, int update_rate, ParamItemType param_type) {
    if(timer_map_.contains(map_key)){
        switch (param_type){
            case ParamItemType::kControl:
                switch(msg_type){
                    case ProtosMessage::PSET:
                        timer_map_[map_key]->start(update_rate);
                        processSetParamReq(map_key);
                        break;
                    case ProtosMessage::PANS:
                        timer_map_[map_key]->stop();
                        break;
                    default:
                        break;
                }
                break;
            case ParamItemType::kUpdate:
                switch(msg_type){
                    case ProtosMessage::PSET:
                        timer_map_[map_key]->start(update_rate);
                        processSetParamReq(map_key);
                        break;
                    case ProtosMessage::PANS:
                        timer_map_[map_key]->start(update_rate);
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
}

void ParamService::makeParamTimer(const QString &mapKey){
    if(!timer_map_.contains(mapKey)) {
        auto* newTim = new QTimer(this);
        timer_map_.insert(mapKey, newTim);
        timer_map_[mapKey]->start(param_map_[mapKey]->getViewUpdateRate());
        connect(newTim, &QTimer::timeout, [this, newTim]() {
            timerFinished(newTim->timerId());
        });
    }
}

QString ParamService::makeMapKey(uchar host, uchar ID){
    return QString("%1_%2").arg(ID,0,16).arg(host,0,16);
}

QString ParamService::makeMapKey(const QJsonObject& jsonObject){
    return QString("%1_%2").arg(jsonObject["ID_"].toInt(),0,16).arg(jsonObject["HostID"].toInt(),0,16);
}

QString ParamService::makeMapKey(const ParamItem& paramItem){
    return QString("%1_%2").arg(paramItem.getParamId(), 0, 16).arg(paramItem.getHostID(), 0, 16);
}

QString ParamService::makeMapKey(const ParamItem* paramItem){
    return QString("%1_%2").arg(paramItem->getParamId(), 0, 16).arg(paramItem->getHostID(), 0, 16);
}

QString ParamService::tableNameToMapKey(QString tableName){
    return tableName.remove("param_0x").remove("host_0x");
}

bool ParamService::removeParam(const QString& mapKey){
    if(!param_map_.contains(mapKey)) return false;
    auto removedType = param_map_[mapKey]->getParamType();
    removeFromAllMaps(mapKey);
    return true;
}

void ParamService::removeFromAllMaps(const QString& mapKey){
    if(param_map_.contains(mapKey))
        param_map_.remove(mapKey);
    if(timer_map_.contains(mapKey)) timer_map_.remove(mapKey);
}

void ParamService::writeParamToFile(ParamItem &param, const QString &event) {
    if(!log_file_->isOpen()){
        auto fileName = QDateTime::currentDateTime().toString(QString("yyyy.MM.dd_hh.mm.ss")).append(".csv");
        auto pathToLogs = QString(CURRENT_BUILD_TYPE_) == "Debug" ? "/../logs" : "/logs";
        log_file_ = new QFile(QCoreApplication::applicationDirPath() + QString("%1/%2").arg(pathToLogs, fileName));
        log_file_->open(QIODevice::ReadWrite);
        text_stream_.setDevice(log_file_);
        text_stream_ << param.getLogToFileHead();
    }
    text_stream_ << param.getLogToFileStr(event);
}

void ParamService::configureTimer(int timerValue){
    write_timer_ = new QTimer();
    connect(write_timer_, SIGNAL(timeout()), this, SLOT(writeTimerUpdate()));
    write_timer_->start(kMSec_write_time_);
}

void ParamService::writeTimerUpdate() {
}

void ParamService::saveParams(QJsonObject& qJsonObject){
    QJsonArray paramArr;
    for(auto& p: param_map_)
        paramArr.append(p->getJsonObject());
    qJsonObject["Params"] = paramArr;
    log_file_->close();
}

void ParamService::loadParams(QJsonObject& qJsonObject) {
    QJsonArray paramArr = qJsonObject["Params"].toArray();
    param_map_.clear();
    timer_map_.clear();
    for (const auto& param : paramArr)
    {
        QJsonObject loadParamJson = param.toObject();
        if(!loadParamJson.isEmpty())
        {
            auto mapKey = makeMapKey(loadParamJson);
            param_map_.insert(mapKey, QSharedPointer<ParamItem>(new ParamItem(loadParamJson)));
            makeParamTimer(mapKey);
        }
    }
}

void ParamService::setParamEvent(const QString &mapKey){
    auto& paramToWrite = param_map_[mapKey];
    auto senderId = paramToWrite->getSenderId() == self_addr_ ? "localHost" : QString("%1").arg(paramToWrite->getSenderId(), 0, 16).toUpper().prepend("0x");
    auto eventStr = QString("SET_PARAM:%1 VALUE:%2 FROM:%3")
            .arg(paramToWrite->getTableName(), paramToWrite->getValue().toString(), senderId);
}

void ParamService::rxMsgHandler(const ProtosMessage &message){
    switch (message.MsgType){
        case ProtosMessage::MsgTypes::PANS:
            processPANSMsg(message);
            break;
        case ProtosMessage::MsgTypes::PSET:
            processSETMsg(message);
            break;
        default:
            break;
    }
}

void ParamService::processPANSMsg(const ProtosMessage& message){
    QString mapKey = makeMapKey(message.GetSenderAddr(), message.GetParamId());
    switch (message.ParamField) {
        case ProtosMessage::VALUE:
            if(!updateParam(message, mapKey))
                addParam(std::move(ParamItem(message, ParamItemType::kUpdate)));
            break;
        case ProtosMessage::UPDATE_RATE:
        case ProtosMessage::CTRL_RATE:
            updateParamViewUpdateRate(mapKey, message.GetParamFieldValue());
        case ProtosMessage::SEND_TIME:
        case ProtosMessage::UPDATE_TIME:
        case ProtosMessage::SEND_RATE:
        case ProtosMessage::CTRL_TIME:
            updateParamUpdateRates(mapKey, message.GetParamFieldValue(), message.ParamField);
        case ProtosMessage::MULT:
        case ProtosMessage::OFFSET:
            updateParamCalibData(mapKey, message.GetParamFieldValue(), message.ParamField);
        default:
            break;
    }
}

void ParamService::processSETMsg(const ProtosMessage& message){
    QString mapKey = makeMapKey(message.GetDestAddr(), message.GetParamId());
    switch (message.ParamField) {
        case ProtosMessage::VALUE:
            if(!updateParam(message, mapKey))
                addParam(std::move(ParamItem(message, ParamItemType::kControl)));
            break;
        case ProtosMessage::UPDATE_RATE:
        case ProtosMessage::CTRL_RATE:
        case ProtosMessage::SEND_TIME:
        case ProtosMessage::UPDATE_TIME:
        case ProtosMessage::SEND_RATE:
        case ProtosMessage::CTRL_TIME:
//            updateParamUpdateRates(mapKey, message.GetParamFieldValue(), message.ParamField);
        case ProtosMessage::MULT:
        case ProtosMessage::OFFSET:
//            updateParamCalibData(mapKey, message.GetParamFieldValue(), message.ParamField);
        default:
            break;
    }
}

void ParamService::updateParamViewUpdateRate(const QString& mapKey, const QVariant& value){
    bool ok;
    auto newUpdateRateValue = (short)value.toInt(&ok);
    if(ok){
        if(newUpdateRateValue > 2900) newUpdateRateValue *= 1.5;
        else if(newUpdateRateValue >= 1200) newUpdateRateValue *= 2.5;
        else newUpdateRateValue *= 4;
        if(param_map_.contains(mapKey))
            param_map_[mapKey]->setViewUpdateRate(newUpdateRateValue);
        if(timer_map_.contains(mapKey))
            timer_map_.value(mapKey)->setInterval(newUpdateRateValue);
    }
}

void ParamService::updateParamUpdateRates(const QString& mapKey, const QVariant& value, uchar paramField){
    bool ok;
    auto newValue = (short)value.toInt(&ok);
    if(ok)
        param_map_[mapKey]->setRateValue(paramField, newValue);
}

void ParamService::updateParamCalibData(const QString& mapKey, const QVariant& value, uchar paramField){
    bool ok;
    auto newValue = value.toDouble(&ok);
    if(ok)
        param_map_[mapKey]->setCalibValue(paramField, newValue);
}

void ParamService::txMsgHandler(const ProtosMessage &message) {

}

void ParamService::setParamValueChanged(const QString& mapKey, const QVariant& value){
    if(!param_map_.contains(mapKey)) return;
    auto param = param_map_[mapKey];
    param->setExpectedValue(value);
    param->setSenderId(self_addr_);
    param->setState(ParamItemStates::kPending);
    param->setLastValueType(ProtosMessage::MsgTypes::PSET);
    emit onChangedParamState(ParamItemType::kUpdate);
    emit changedParamValue(param);
    param->setValue(value);
    sendProtosMsgSetParam(mapKey);
    processSetParamReq(mapKey);
}

void ParamService::setParamValueChanged(uchar Id, uchar Host, const QVariant& value){
    auto mapKey = makeMapKey(Host, Id);
    if(!param_map_.contains(mapKey))
        addParam(Id, Host, ParamItemType::kControl);
    auto param = param_map_[mapKey];
    param->setExpectedValue(value);
    param->setSenderId(self_addr_);
    param->setState(ParamItemStates::kPending);
    param->setLastValueType(ProtosMessage::MsgTypes::PSET);
    emit onChangedParamState(ParamItemType::kUpdate);
    emit changedParamValue(param);
    param->setValue(value);
    sendProtosMsgSetParam(mapKey);
    processSetParamReq(mapKey);
}

void ParamService::processSetParamReq(const QString& mapKey){
    if(request_on_set_){
        auto param = param_map_[mapKey];
        ProtosMessage msg(param->getHostID(), self_addr_, param->getParamId(), ProtosMessage::MsgTypes::PREQ, ProtosMessage::ParamFields::VALUE);
        socket_adapter_->SendMsg(msg);
    }
}

void ParamService::sendProtosMsgSetParam(const QString &mapKey){
    auto param = param_map_[mapKey];
    ProtosMessage msg(param->getHostID(), self_addr_, param->getParamId(), short(param->getValue().toInt()), ProtosMessage::MsgTypes::PSET);
    socket_adapter_->SendMsg(msg);
}

void ParamService::timerFinished(int timID) {
    for(auto it=timer_map_.begin(); it != timer_map_.end(); it++){
        if(it.value()->timerId() == timID){
            it.value()->stop();
            if(param_map_.contains(it.key())) {
                param_map_[it.key()]->timeoutUpdate();
                emit onChangedParamState(ParamItemType::kUpdate);
            }
            break;
        }
    }
}

void ParamService::setSelfAddr(uchar incomeSelfAddr) {
    self_addr_ = incomeSelfAddr;
}

uchar ParamService::getSelfAddr() const {
    return self_addr_;
}

void ParamService::setWriteToFile(bool incomeWriteToFile) {
    ParamService::write_to_file_ = incomeWriteToFile;
}

bool ParamService::isWriteToFile() const {
    return write_to_file_;
}

void ParamService::closeAll() {
    log_file_->close();
}

QSet<uchar> ParamService::getAllHostsInSet() {
   auto retVal = QSet<uchar>();
   for(const auto& param : param_map_)
       retVal.insert(param->getHostID());
   return retVal;
}

void ParamService::configParam(const QString &mapKey){
    if(param_map_.contains(mapKey)){
        auto param = param_map_[mapKey];
    }
}

void ParamService::setReqOnSet(bool _reqOnSet) {
    request_on_set_ = _reqOnSet;
}

bool ParamService::isReqOnSet() const {
    return request_on_set_;
}

void ParamService::removeAllParams() {
    for(const auto& key: param_map_.keys())
        removeFromAllMaps(key);
    param_map_.clear();
    timer_map_.clear();
    emit needToResetModels();
}

QStringList ParamService::getAllParamsStrList() {
    auto retVal = QStringList();
    for(const auto& param : param_map_)
        retVal.append(param->getTableName());
    return retVal;
}

QSharedPointer<ParamItem> ParamService::getParam(const QString& tableName){
    return param_map_.value(tableNameToMapKey(tableName));
}

bool ParamService::containsParam(const QString& mapKey) {
    return param_map_.contains(mapKey);
}

template<typename T>
void ParamService::makeParamServiceMsg(ParamItem* paramItem, ProtosMessage::ParamFields field, T value, bool set){
    ProtosMessage message = ProtosMessage(
            paramItem->getHostID(),
            1,
            paramItem->getParamId(),
            set ? ProtosMessage::PSET : ProtosMessage::PREQ,
            field,
            QVariant()
    );
    if(set) message.SetParamValue(value, true);
    socket_adapter_->SendMsg(message);
}

template<typename T>
void ParamService::sendServiceMsgSet(ParamItem *paramItem, const T& value, ProtosMessage::ParamFields field){
    makeParamServiceMsg(paramItem, field, value, true);
}

void ParamService::sendServiceMsgReq(ParamItem *paramItem, ProtosMessage::ParamFields field){
    makeParamServiceMsg(paramItem, field);
}

template void ParamService::sendServiceMsgSet<float>
        (ParamItem *paramItem, const float& value, ProtosMessage::ParamFields field);
template void ParamService::sendServiceMsgSet<short>
        (ParamItem *paramItem, const short& value, ProtosMessage::ParamFields field);