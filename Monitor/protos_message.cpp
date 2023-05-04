#include "protos_message.h"

ProtosMessage::ProtosMessage() : CanId(0), Dlc(0)
{
	//qRegisterMetaType <ProtosMessage> ("ProtosMessage");
	for (unsigned char & i : Data) i = 0;
}

ProtosMessage::ProtosMessage(uchar destAddr, uchar senderAddr, ProtosMessage::MsgTypes msgType, uchar dlc, uchar data0, uchar data1, uchar data2, uchar data3, uchar data4, uchar data5, uchar data6, uchar data7, uchar data8)	:
	CanId(0)
{
    Dlc = dlc;
    DestAddr = destAddr;
    SenderAddr = senderAddr;
    MsgType = msgType;
	Data[0] = data0;
	Data[1] = data1;
	Data[2] = data2;
	Data[3] = data3;
	Data[4] = data4;
	Data[5] = data5;
	Data[6] = data6;
	Data[7] = data7;
}

ProtosMessage::ProtosMessage(uchar destAddr, uchar senderAddr, uchar paramId, ProtosMessage::MsgTypes msgType, ProtosMessage::ParamFields paramField, QVariant fieldValue) : CanId(0)
{
	SetParamField(paramField, paramId, msgType, fieldValue, destAddr, senderAddr);
}

ProtosMessage::ProtosMessage(uchar destAddr, uchar senderAddr, uchar paramId, char charValue, ProtosMessage::MsgTypes msgType) : CanId(0)
{
	SetParamValue(charValue, paramId, msgType, destAddr, senderAddr);
}

ProtosMessage::ProtosMessage(uchar destAddr, uchar senderAddr, uchar paramId, short shortValue, ProtosMessage::MsgTypes msgType) : CanId(0)
{
	SetParamValue(shortValue, paramId, msgType, destAddr, senderAddr);
}

ProtosMessage::ProtosMessage(uchar destAddr, uchar senderAddr, uchar paramId, int intValue, ProtosMessage::MsgTypes msgType) : CanId(0)
{
	SetParamValue(intValue, paramId, msgType, destAddr, senderAddr);
}

ProtosMessage::ProtosMessage(uchar destAddr, uchar senderAddr, uchar paramId, float floatValue, ProtosMessage::MsgTypes msgType) : CanId(0)
{
	SetParamValue(floatValue, paramId, msgType, destAddr, senderAddr);
}

ProtosMessage::ProtosMessage(uchar destAddr, uchar senderAddr, uchar paramId, const QString& stringValue, ProtosMessage::MsgTypes msgType) : CanId(0)
{
	SetParamValue(stringValue, paramId, msgType, destAddr, senderAddr);
}

ProtosMessage::ProtosMessage(const QByteArray& byteArray) : CanId(0), Dlc(0)
{
	for (int i = 0; i < 8; i++)	Data[i] = 0;
	FromByteArray(byteArray);
}

ProtosMessage::ProtosMessage(const QString& string) : CanId(0), Dlc(0)
{
	for (int i = 0; i < 8; i++)	Data[i] = 0;
	FromString(string);
}

ProtosMessage::~ProtosMessage()
{
}

void ProtosMessage::FromByteArray(const QByteArray& byteArray)
{	
	int maxSize = IdLng + 8;
	int byteArraySize = byteArray.size();
	if (byteArraySize < maxSize) 
		maxSize = byteArraySize;
	Dlc = byteArraySize - IdLng;
	
	for (int byteIndex = 0; byteIndex < maxSize; byteIndex++)
	{
		if (byteIndex < IdLng)
			IdBytes[byteIndex] = byteArray.at(byteIndex);
		else
			Data[byteIndex - IdLng] = byteArray.at(byteIndex);		
	}
}

bool ProtosMessage::FromString(const QString& st)
{
	QStringList stList(st.split('.'));
	if (stList.size() != 3)
		return false;

	bool isConvOk;
	int dlc = stList.at(1).toInt(&isConvOk, 16);
	QString idSt = stList.at(0).trimmed();
	QString dataSt = stList.at(2).trimmed();
	if (!isConvOk || idSt.size() != 8 || 2 * dlc != dataSt.size())
		return false;

	Dlc = dlc;
	int len = IdLng + dlc;
	uchar temp[12];
	for (int i = 0; i < 4; i++)
	{
		temp[i] = idSt.right(2).toInt(&isConvOk, 16);
		if (!isConvOk)
			return false;
		else
			idSt.chop(2);
	}
	for (int i = IdLng; i < len; i++)
	{
		temp[i] = dataSt.left(2).toInt(&isConvOk, 16);
		if (!isConvOk)
			return false;
		else
			dataSt.remove(0, 2);
	}
	for (int i = 0; i < len; i++)
		operator[](i) = temp[i];
	return true;
}

uchar ProtosMessage::GetCmdId() const
{
	switch (MsgType)
	{
	case MsgTypes::MCMD:
	case MsgTypes::MANS:
	case MsgTypes::SCMD:
	case MsgTypes::SANS:
	case MsgTypes::MERR:
	case MsgTypes::SERR:
		return CmdId;
	default:
		return 0;		
	}
}

QString ProtosMessage::GetErrorMsg() const
{
	QString errorMsg, cmd, id, err;
	
	cmd = QString("%1").arg(CmdId, ToHexStr).toUpper();
	err = QString("%1").arg(Data[1], ToHexStr).toUpper();
	id  = QString("%1").arg(Data[2], ToHexStr).toUpper();
	switch (MsgType)
	{
	case MERR:		
		return QObject::tr("Got MERR 0x%1 on 0x%2 cmd from ID 0x%3").arg(err, cmd, id);		
	case SERR:
		errorMsg = QObject::tr("Got SERR 0x%1 on 0x%2 cmd from ID 0x%3").arg(err, cmd, id);

		switch (CmdId)
		{
		case 0xA0: //OWSearch cmd has no error codes 
			break;
		case 0xA2:
			switch (Data[1])
			{
			case 0x01:
				errorMsg = QObject::tr("1-Wire parameter with ID 0x%1 not found").arg(id);
				break;
			}
		case 0xA5: //OWParams request cmd has no error codes
			break;
		case 0xA7: // ERR for Th, Tl, Resolition set
			break;
		case 0xA8: // ERR for Th, Tl, Resolution request
			break;
		default:
			break;
		}
		return errorMsg;
	case PERR:		
//		id.arg(Data[0], ToHexStr).toUpper();
//		err.arg(Data[1], ToHexStr).toUpper();
		return QObject::tr("Got PERR 0x%1 from ID 0x%2 ").arg(err, id);		
	default:
		return "";
	}
}

QString ProtosMessage::GetMsgType() const
{
	return GetMsgTypeName(MsgType);
}

QString ProtosMessage::GetMsgTypeName(uchar msgType)
{
	switch (msgType)
	{
	case NONE: return "NONE";
	case MCMD: return "M.CMD";
	case SCMD: return "S.CMD";
	case MANS: return "M.ANS";
	case SANS: return "S.ANS";
	case MERR: return "M.ERR";
	case SERR: return "S.ERR";
	case PREQ: return "P.REQ";
	case PANS: return "P.ANS";
	case PSET: return "P.CONTROL";
	case PERR: return "P.ERR";
	default:   return "UNDEFINED";
	}
}

QString ProtosMessage::GetParamField() const
{
	return GetParamFieldName(ParamField);
}

uchar ProtosMessage::GetParamFieldLength() const
{
	switch (MsgType)
	{
	case MsgTypes::PANS:
	case MsgTypes::PSET:
		switch (ParamFieldType)
		{
		case ParamFieldTypes::VOID:		return 0;
		case ParamFieldTypes::CHAR:		return 1;
		case ParamFieldTypes::SHORT:	return 2;
		case ParamFieldTypes::LONG:		return 4;
		case ParamFieldTypes::FLOAT:	return 4;
		case ParamFieldTypes::STRING:	return 6;
		case ParamFieldTypes::TIME:		return 4;
		case ParamFieldTypes::RESERVED:	return 0;
		default: return 0;
		}
	default:
		return 0;
	}
}

QString ProtosMessage::GetParamFieldName(uchar field)
{
	switch (field)
	{
	case ID:	return "ID";
	case TYPE:	return "TYPE";
	case VALUE:	return "VALUE";
	case SEND_RATE:	return "SEND_RATE";
	case READ_ONLY:	return "READ_ONLY";
	case SEND_TIME:	return "SEND_TIME";
	case UPDATE_BEFORE_READ: return "UPDATE_BEFORE_READ";
	case UPDATE_RATE: return "UPDATE_RATE";
	case UPDATE_TIME: return "UPDATE_TIME";
	case CTRL_RATE:	return "CTRL_RATE";
	case CTRL_TIME:	return "CTRL_TIME";
	case SET_ACK:	return "SET_ACK";
	case CTRL_ACK:	return "CTRL_ACK";
	case MULT:		return "MULT";
	case OFFSET:	return "OFFSET";
	default: return "UNDEFINED";
	}
}

QString ProtosMessage::GetParamFieldTypeName(uchar fieldType)
{
	switch (fieldType)
	{
	case VOID:	return "VOID";
	case CHAR:	return "CHAR";
	case SHORT: return "SHORT";
	case LONG:	return "LONG";
	case FLOAT: return "FLOAT";
	case STRING:return "STRING";
	case TIME:	return "TIME";
	case RESERVED: return "RESERVED";
	default: return "UNDEFINED";
	};
}

QVariant ProtosMessage::GetParamFieldValue() const
{
	QString st(""); 	
	
	switch (MsgType)
	{
	case MsgTypes::PANS:
	case MsgTypes::PSET:
		switch (ParamFieldType)
		{
		case ParamFieldTypes::VOID:
			return 0;
		case ParamFieldTypes::CHAR:			
			return QVariant(CharField & 0x80 ? CharField | 0xffffff00 : CharField);			
		case ParamFieldTypes::SHORT:
			return QVariant(ShortField & 0x8000 ? ShortField | 0xffff0000 : ShortField);	
		case ParamFieldTypes::LONG:	
			return QVariant(LongField);		
		case ParamFieldTypes::FLOAT:				
			return QVariant(FloatField);
		case ParamFieldTypes::STRING:				
			for (int i = 2; i < 8; i++)
				st.append(Data[i]);
			return QVariant(st);						
		case ParamFieldTypes::TIME:			
			return QVariant(QTime(0, 0, 0, 0).addMSecs(UlongField));
		case ParamFieldTypes::RESERVED:
			return QVariant();
		default:
			return QVariant();
		}
	default:
		return QVariant();
	}
}

uchar ProtosMessage::GetParamId() const
{
	switch (MsgType)
	{
	case MsgTypes::MCMD:
	case MsgTypes::MANS:
	case MsgTypes::SCMD:
	case MsgTypes::SANS:
		return Data[1];
	
	case MsgTypes::MERR:
	case MsgTypes::SERR:
		return Data[2];
	
	case MsgTypes::PREQ:
	case MsgTypes::PSET:
	case MsgTypes::PANS:
	case MsgTypes::PERR:
		return ParamId;
	
	default:
		return 0;
	}
}

uchar ProtosMessage::GetSenderAddr() const
{
    return SenderAddr;
}

uchar ProtosMessage::GetDestAddr() const{
    return DestAddr;
}

bool ProtosMessage::IsCorrect(QString& errorMsg) const
{
	QString arg1("%1"), arg2("%1"), arg3("%1"), arg4("%1");
	uchar wantedDlc = 0;
	errorMsg = "";

	switch (MsgType)
	{
	case MCMD:		
	case SCMD:	
	case MANS:		
	case SANS:		
		return true;

	case MERR:		
	case SERR:
		if (Dlc >= 3)
			return true;
		arg1 = GetMsgType();
//		arg2.arg(Data[1], ToHexStr).toUpper();
//		arg3.arg(GetCmdId(), ToHexStr).toUpper();
		arg4 = Dlc;
		errorMsg = QObject::tr("Got %1: 0x%2 for 0x%3 cmd with wrong DLC (wanted not less than 3, got %4)").arg(arg1, arg2, arg3, arg4);
		break;
	
	case PREQ:
	case PERR:
		if (Dlc == 2)
			return true;
		errorMsg = QObject::tr("Got %1 with wrong DLC (wanted 2, got %2) ").arg(GetMsgType(), Dlc);
		break;
	
	case PANS:		
	case PSET:
		wantedDlc = GetParamFieldLength() + 2;
		if (wantedDlc == Dlc)
			return true;
		
		errorMsg = QObject::tr("Got %1 with DLC not corresponding MsgType (wanted %2, got %3) ").arg(GetMsgType(), wantedDlc, Dlc);				
		break;
	
	default:
		errorMsg = QObject::tr("Got unknown message type (0x%1) ").arg(MsgType, ToHexStr);				
	}
	
	errorMsg.append(ToString());
	return false;
}

ProtosMessage& ProtosMessage::SetCanIdFields(ProtosMessage::MsgTypes msgType, uchar destAddr, uchar senderAddr)
{
	if (msgType != NONE) MsgType = msgType;
	if (senderAddr) SenderAddr = senderAddr;
	if (destAddr)	DestAddr = destAddr;
	return *this;
}

ProtosMessage& ProtosMessage::SetParamField(ProtosMessage::ParamFields field, uchar paramId, ProtosMessage::MsgTypes msgType, QVariant fieldValue, uchar destAddr, uchar senderAddr)
{
	if (msgType == MsgTypes::MCMD || msgType == MsgTypes::MANS || msgType == MsgTypes::MERR ||
		msgType == MsgTypes::SCMD || msgType == MsgTypes::SANS || msgType == MsgTypes::SERR)
		return *this;

	SetCanIdFields(msgType, destAddr, senderAddr);
	if (paramId)
		ParamId	= paramId;
	ParamField	= field;
	
	switch (MsgType)
	{
	case MsgTypes::PSET:
	case MsgTypes::PANS:
		switch (field)
		{
		case ParamFields::CTRL_ACK:
		case ParamFields::SET_ACK:
		case ParamFields::UPDATE_BEFORE_READ:
			Dlc = 3;
			ParamFieldType = ParamFieldTypes::CHAR;
			if (fieldValue != QVariant()) 
				CharField = fieldValue.toInt() & 0xff;			
			break;
		
		case ParamFields::CTRL_RATE:
		case ParamFields::SEND_RATE:
		case ParamFields::UPDATE_RATE:		
			Dlc = 4;
			ParamFieldType = ParamFieldTypes::SHORT;
			if (fieldValue != QVariant())	
				ShortField = fieldValue.toInt() & 0xffff;			
			break;
		
		case ParamFields::MULT:
		case ParamFields::OFFSET:
			Dlc = 6;
			ParamFieldType = ParamFieldTypes::FLOAT;
			if (fieldValue != QVariant())
				FloatField = fieldValue.toFloat();
			break;
			// all other param fields are read only
		}
		break;
	
	case MsgTypes::PREQ:
		Dlc = 2;
		switch (field)
		{
		case ParamFields::CTRL_ACK:
		case ParamFields::ID:
		case ParamFields::READ_ONLY:
		case ParamFields::SET_ACK:
		case ParamFields::TYPE:
		case ParamFields::UPDATE_BEFORE_READ:			
			ParamFieldType = ParamFieldTypes::CHAR;			
			break;
		
		case ParamFields::CTRL_RATE:
		case ParamFields::CTRL_TIME:
		case ParamFields::SEND_RATE:
		case ParamFields::SEND_TIME:
		case ParamFields::UPDATE_RATE:
		case ParamFields::UPDATE_TIME:					
			ParamFieldType = ParamFieldTypes::SHORT;
			break;	
		
		case ParamFields::MULT:
		case ParamFields::OFFSET:
			ParamFieldType = ParamFieldTypes::FLOAT;
			break;		
		}
		break;
	
	case MsgTypes::PERR:
		Dlc = 2;
		if (fieldValue != QVariant())
			Data[1] = fieldValue.toInt() & 0xff;
		break;
	}	
	
	return *this;
}

ProtosMessage& ProtosMessage::SetParamValue(char charValue, uchar paramId, ProtosMessage::MsgTypes msgType, uchar destAddr, uchar senderAddr)
{
	SetCanIdFields(msgType, destAddr, senderAddr);	
	Dlc = 3;
	if (paramId)
		ParamId = paramId;
	ParamField = ParamFields::VALUE;
	ParamFieldType = ParamFieldTypes::CHAR;
	CharField = charValue;	

	return *this;
}

ProtosMessage& ProtosMessage::SetParamValue(short shortValue, uchar paramId, ProtosMessage::MsgTypes msgType, uchar destAddr, uchar senderAddr)
{
	SetCanIdFields(msgType, destAddr, senderAddr);	
	Dlc	= 4;
	if (paramId)
		ParamId = paramId;
	ParamField = ParamFields::VALUE;
	ParamFieldType = ParamFieldTypes::SHORT;
	ShortField = shortValue;	

	return *this;
}

ProtosMessage& ProtosMessage::SetParamValue(int intValue, uchar paramId, ProtosMessage::MsgTypes msgType, uchar destAddr, uchar senderAddr)
{
	SetCanIdFields(msgType, destAddr, senderAddr);
	Dlc	= 6;	
	if (paramId)
		ParamId = paramId;
	ParamField = ParamFields::VALUE;
	ParamFieldType = ParamFieldTypes::LONG;
	LongField = intValue;	

	return *this;
}

void ProtosMessage::SetParamValue(short shortValue, bool ok)
{
    if(!ok) return;
    Dlc	= 6;
    ParamFieldType = ParamFieldTypes::SHORT;
    LongField = shortValue;
}

void ProtosMessage::SetParamValue(float floatValue, bool ok)
{
    if(!ok) return;
    Dlc	= 6;
    ParamFieldType = ParamFieldTypes::FLOAT;
    FloatField = floatValue;
}

ProtosMessage& ProtosMessage::SetParamValue(float floatValue, uchar paramId, ProtosMessage::MsgTypes msgType, uchar destAddr, uchar senderAddr)
{
	SetCanIdFields(msgType, destAddr, senderAddr);
	Dlc	= 6;
	if (paramId)
		ParamId = paramId;
	ParamField = ParamFields::VALUE;
	ParamFieldType = ParamFieldTypes::FLOAT;
	FloatField = floatValue;	

	return *this;
}

ProtosMessage& ProtosMessage::SetParamValue(const QString& stringValue, uchar paramId, ProtosMessage::MsgTypes msgType, uchar destAddr, uchar senderAddr)
{
	SetCanIdFields(msgType, destAddr, senderAddr);
	Dlc	= stringValue.size() > 6 ? 6 : stringValue.size() + 2;	
	if (paramId)
		ParamId = paramId;
	ParamField = ParamFields::VALUE;
	ParamFieldType = ParamFieldTypes::STRING;

	int len = Dlc - 2;
	for (int i = 0; i < len; i++)
		Data[i + 2] = stringValue.at(i).toLatin1();

	return *this;
}

QByteArray ProtosMessage::ToByteArray() const
{
	QByteArray byteArray(IdLng + Dlc, 0);
	for (int i = 0; i < 4; i++)
		byteArray[i] = IdBytes[i];	
	for (int i = 0; i < Dlc; i++)
		byteArray[IdLng + i] = Data[i];
	return byteArray;
}

QString ProtosMessage::ToString() const
{
	QString st(QString("%1%2%3%4.%5.").arg(IdBytes[3], ToHexStr).arg(IdBytes[2], ToHexStr).arg(IdBytes[1], ToHexStr).arg(IdBytes[0], ToHexStr).arg(Dlc));
	for (int i = 0; i < Dlc; i++)
		st.append(QString("%1").arg(Data[i], ToHexStr));
	return st.toUpper();
}

uchar& ProtosMessage::operator[](const uchar& index)
{
	if (index < IdLng)
		return IdBytes[index];
	else if (index < IdLng + 8)
		return Data[index - IdLng];
	else
		return IdBytes[0];
}

