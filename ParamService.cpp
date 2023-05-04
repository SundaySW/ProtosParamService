//
// Created by outlaw on 28.10.2022.
//

#include "ParamService.h"

#ifdef _BUILD_TYPE_
#define CURRENT_BUILD_TYPE_ _BUILD_TYPE_
#else
#define CURRENT_BUILD_TYPE_ "CHECK CMAKE"
#endif

ParamService::ParamService(QJsonObject &inJson) :
        qJsonObject(inJson),
        logFile(new QFile()),
        writeToFile(false)
{
    socketAdapter.AddTxMsgHandler([this](const ProtosMessage& txMsg) { txMsgHandler(txMsg);});
    socketAdapter.AddRxMsgHandler([this](const ProtosMessage& txMsg) { rxMsgHandler(txMsg);});
    loadParams();
}

bool ParamService::addParam(uchar incomeID, uchar incomeHostAddr, ParamItemType incomeType) {
    return addParam(ParamItem(incomeID, incomeHostAddr, incomeType), false);
}

bool ParamService::addParam(ParamItem &&paramItem, bool addToDB){
    auto mapKey = makeMapKey(paramItem);
    if(paramsMap.contains(mapKey)) return false;
    paramsMap.insert(mapKey, paramItem);
    auto* appendParam = (ParamItem*)&paramsMap[mapKey];
    auto type = appendParam->getParamType();
    makeParamTimer(mapKey);
    emit addedParamFromLine(type);
    emit addedParamFromLine(paramsMap[mapKey]);
    return true;
}

bool ParamService::updateParam(const ProtosMessage &message, const QString &mapKey) {
    if(paramsMap.contains(mapKey)){
        auto& paramToUpdate = paramsMap[mapKey];
        paramToUpdate.update(message);
        emit changedParamState(paramToUpdate.getParamType());
        emit changedParamValue(paramToUpdate);
        manageTimersWhileUpdate(mapKey, message.MsgType, paramToUpdate.getUpdateRate(), paramToUpdate.getParamType());
        return true;
    }
    return false;
}

void ParamService::manageTimersWhileUpdate(const QString &mapKey, uchar msgType, int updateRate, int paramType) {
    if(timerMap.contains(mapKey)){
        switch (paramType){
            case CONTROL:
                switch(msgType){
                    case ProtosMessage::PSET:
                        timerMap[mapKey]->start(updateRate);
                        processSetParamReq(mapKey);
                        break;
                    case ProtosMessage::PANS:
                        timerMap[mapKey]->stop();
                        break;
                    default:
                        break;
                }
                break;
            case UPDATE:
                switch(msgType){
                    case ProtosMessage::PSET:
                        timerMap[mapKey]->start(updateRate);
                        processSetParamReq(mapKey);
                        break;
                    case ProtosMessage::PANS:
                        timerMap[mapKey]->start(updateRate);
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
    if(!timerMap.contains(mapKey)) {
        auto* newTim = new QTimer(this);
        timerMap.insert(mapKey, newTim);
        timerMap[mapKey]->start(paramsMap[mapKey].getUpdateRate());
        connect(newTim, &QTimer::timeout, [this, newTim]() {
            timerFinished(newTim->timerId());
        });
    }
}

QString ParamService::makeMapKey(uchar host, uchar ID){
    return QString("%1_%2").arg(ID,0,16).arg(host,0,16);
}

QString ParamService::makeMapKey(const ParamItem& paramItem){
    return QString("%1_%2").arg(paramItem.getParamId(), 0, 16).arg(paramItem.getHostID(), 0, 16);
}

bool ParamService::removeParam(const QString& mapKey) {
    if(!paramsMap.contains(mapKey)) return false;
    auto removedType = paramsMap[mapKey].getParamType();
    removeFromAllMaps(mapKey);
    return true;
}

void ParamService::removeFromAllMaps(const QString& mapKey){
    if(paramsMap.contains(mapKey))
        paramsMap.remove(mapKey);
    if(timerMap.contains(mapKey)) timerMap.remove(mapKey);
}

void ParamService::writeParamToFile(ParamItem &param, const QString &event) {
    if(!logFile->isOpen()){
        auto fileName = QDateTime::currentDateTime().toString(QString("yyyy.MM.dd_hh.mm.ss")).append(".csv");
        auto pathToLogs = QString(CURRENT_BUILD_TYPE_) == "Debug" ? "/../logs" : "/logs";
        logFile = new QFile(QCoreApplication::applicationDirPath() + QString("%1/%2").arg(pathToLogs, fileName));
        logFile->open(QIODevice::ReadWrite);
        textStream.setDevice(logFile);
        textStream << param.getLogToFileHead();
    }
    textStream << param.getLogToFileStr(event);
}

void ParamService::configureTimer(int timerValue){
    writeTimer = new QTimer();
    connect(writeTimer, SIGNAL(timeout()), this, SLOT(writeTimerUpdate()));
    writeTimer->start(mSecWriteTimer);
}

void ParamService::writeTimerUpdate() {
}

void ParamService::saveParams() {
    QJsonArray paramArr;
    for(auto& p: paramsMap)
        paramArr.append(p.getJsonObject());
    qJsonObject["Params"] = paramArr;
    logFile->close();
}

void ParamService::loadParams() {
    QJsonArray paramArr = qJsonObject["Params"].toArray();
    paramsMap.clear();
    timerMap.clear();
    for (const auto& param : paramArr)
    {
        QJsonObject loadParam = param.toObject();
        if (param != QJsonObject())
        {
            ParamItem item(loadParam);
            auto mapKey = makeMapKey(item);
            item.setState(OFFLINE);
            paramsMap.insert(mapKey, item);
            makeParamTimer(mapKey);
        }
    }
}

SocketAdapter& ParamService::getSocketAdapter() {
    return socketAdapter;
}

bool ParamService::isSocketConnected(){
    return socketAdapter.IsConnected();
}

void ParamService::setParamEvent(const QString &mapKey){
    auto& paramToWrite = paramsMap[mapKey];
    auto senderId = paramToWrite.getSenderId() == selfAddr ? "localHost" : QString("%1").arg(paramToWrite.getSenderId(),0,16).toUpper().prepend("0x");
    auto eventStr = QString("SET_PARAM:%1 VALUE:%2 FROM:%3")
            .arg(paramToWrite.getTableName(), paramToWrite.getValue().toString(), senderId);
}

void ParamService::rxMsgHandler(const ProtosMessage &message){
    switch (message.MsgType){
        case ProtosMessage::MsgTypes::PANS:
            processPANSMsg(message);
            break;
        case ProtosMessage::MsgTypes::PSET:
            processPSETMsg(message);
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
                addParam(std::move(ParamItem(message, UPDATE)));
            break;
        case ProtosMessage::UPDATE_RATE:
        case ProtosMessage::CTRL_RATE:
            updateParamUpdateRate(mapKey, message.GetParamFieldValue());
        case ProtosMessage::SEND_TIME:
        case ProtosMessage::UPDATE_TIME:
        case ProtosMessage::SEND_RATE:
        case ProtosMessage::CTRL_TIME:
        case ProtosMessage::MULT:
        case ProtosMessage::OFFSET:
            break;
        default:
            break;
    }
}

void ParamService::processPSETMsg(const ProtosMessage& message){
    QString mapKey = makeMapKey(message.GetDestAddr(), message.GetParamId());
    switch (message.ParamField) {
        case ProtosMessage::VALUE:
            if(!updateParam(message, mapKey))
                addParam(std::move(ParamItem(message, CONTROL)));
            break;
        case ProtosMessage::UPDATE_RATE:
        case ProtosMessage::CTRL_RATE:
        case ProtosMessage::SEND_TIME:
        case ProtosMessage::UPDATE_TIME:
        case ProtosMessage::SEND_RATE:
        case ProtosMessage::CTRL_TIME:
        case ProtosMessage::MULT:
        case ProtosMessage::OFFSET:
            break;
        default:
            break;
    }
}

void ParamService::updateParamUpdateRate(const QString& mapKey, const QVariant& value){
    bool ok;
    auto newUpdateRateValue = (short)value.toInt(&ok);
    if(ok){
        if(newUpdateRateValue > 2900) newUpdateRateValue *= 1.5;
        else if(newUpdateRateValue >= 1200) newUpdateRateValue *= 2.5;
        else newUpdateRateValue *= 4;
        if(paramsMap.contains(mapKey))
            paramsMap[mapKey].setUpdateRate(newUpdateRateValue);
        if(timerMap.contains(mapKey))
            timerMap.value(mapKey)->setInterval(newUpdateRateValue);
    }
}

void ParamService::txMsgHandler(const ProtosMessage &message) {

}

void ParamService::setParamValueChanged(const QString& mapKey, const QVariant& value){
    if(!paramsMap.contains(mapKey)) return;
    auto& param = paramsMap[mapKey];
    param.setExpectedValue(value);
    param.setSenderId(selfAddr);
    param.setState(PENDING);
    param.setLastValueType(ProtosMessage::MsgTypes::PSET);
    emit changedParamState(UPDATE);
    emit changedParamValue(param);
    param.setValue(value);
    sendProtosMsgSetParam(mapKey);
    processSetParamReq(mapKey);
}

void ParamService::processSetParamReq(const QString& mapKey){
    if(reqOnSet){
        auto& param = paramsMap[mapKey];
        ProtosMessage msg(param.getHostID(), selfAddr, param.getParamId(), ProtosMessage::MsgTypes::PREQ, ProtosMessage::ParamFields::VALUE);
        socketAdapter.SendMsg(msg);
    }
}

void ParamService::sendProtosMsgSetParam(const QString &mapKey) {
    auto& param = paramsMap[mapKey];
    ProtosMessage msg(param.getHostID(), selfAddr, param.getParamId(), short(param.getValue().toInt()), ProtosMessage::MsgTypes::PSET);
    socketAdapter.SendMsg(msg);
}

void ParamService::timerFinished(int timID) {
    for(auto it=timerMap.begin(); it!=timerMap.end(); it++){
        if(it.value()->timerId() == timID){
            it.value()->stop();
            if(paramsMap.contains(it.key())) {
                paramsMap[it.key()].timeoutUpdate();
                emit changedParamState(UPDATE);
            }
            break;
        }
    }
}

void ParamService::setSelfAddr(uchar incomeSelfAddr) {
    selfAddr = incomeSelfAddr;
}

uchar ParamService::getSelfAddr() const {
    return selfAddr;
}

void ParamService::setWriteToFile(bool incomeWriteToFile) {
    ParamService::writeToFile = incomeWriteToFile;
}

bool ParamService::isWriteToFile() const {
    return writeToFile;
}

void ParamService::closeAll() {
    logFile->close();
}

QSet<uchar> ParamService::getAllHostsInSet() {
   auto retVal = QSet<uchar>();
   for(const auto& param : paramsMap)
       retVal.insert(param.getHostID());
   return retVal;
}

void ParamService::configParam(const QString &mapKey){
    if(paramsMap.contains(mapKey)){
        auto& param = paramsMap[mapKey];
    }
}

void ParamService::setReqOnSet(bool _reqOnSet) {
    reqOnSet = _reqOnSet;
}

bool ParamService::isReqOnSet() const {
    return reqOnSet;
}

void ParamService::removeAllParams() {
    for(const auto& key: paramsMap.keys())
        removeFromAllMaps(key);
    paramsMap.clear();
    timerMap.clear();
    emit needToResetModels();
}

QSet<QString> ParamService::getAllParams() {
    auto retVal = QSet<QString>();
    for(const auto& param : paramsMap)
        retVal.insert(param.getTableName());
    return retVal;
}
