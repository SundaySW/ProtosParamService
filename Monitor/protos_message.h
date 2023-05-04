#ifndef PROTOS_MESSAGE_H
#define PROTOS_MESSAGE_H

#define ToHexStr 2, 16, QChar('0')

#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QVariant>
#include <QVector>

//namespace {
//	const QString MsgTypeNames[11] = 
//	{ 
//		"UNDEFINED", 
//		"M.CMD", 
//		"S.CMD", 
//		"M.ANS", 
//		"S.ANS", 
//		"M.ERR", 
//		"S.ERR", 
//		"P.REQ", 
//		"P.ANS", 
//		"P.CONTROL",
//		"P.ERR" 
//	};
//	const QString ParamFieldNames[13] = 
//	{ 
//		"ID", 
//		"TYPE", 
//		"VALUE", 
//		"SEND_RATE",
//		"READ_ONLY", 
//		"SEND_TIME", 
//		"UPDATE_BEFORE_READ", 
//		"UPDATE_RATE", 
//		"UPDATE_TIME", 
//		"CTRL_RATE", 
//		"CTRL_TIME", 
//		"SET_ACK", 
//		"CTRL_ACK" 
//	};
//} 

struct ProtosMessage
{
	enum ProtocolTypes
	{
		PROTOS	= 0,
		RAW		= 1,
		BOOT    = 1
	};
	enum MsgTypes
	{
		NONE = 0,
		MCMD = 0x01,
		SCMD = 0x02,
		MANS = 0x03,
		SANS = 0x04,
		MERR = 0x05,
		SERR = 0x06,
		PREQ = 0x07,
		PANS = 0x08,
		PSET = 0x09,
		PERR = 0x0A
	};

	enum ParamFieldTypes
	{
		VOID	 = 0x00,
		CHAR	 = 0x01,
		SHORT	 = 0x02,
		LONG	 = 0x03,
		FLOAT	 = 0x04,
		STRING	 = 0x05,
		TIME	 = 0x06,
		RESERVED = 0x07
	};

	enum ParamFields
	{
		ID			= 0x00,
		TYPE		= 0x01,
		VALUE		= 0x02,
		SEND_RATE	= 0x04,
		READ_ONLY	= 0x05,
		SEND_TIME	= 0x06,
		UPDATE_BEFORE_READ = 0x07,
		UPDATE_RATE	= 0x08,
		UPDATE_TIME	= 0x09,
		CTRL_RATE	= 0x0A,
		CTRL_TIME	= 0x0B,
		SET_ACK		= 0x0C,
		CTRL_ACK	= 0x0D,
		RES			= 0x0E,
		MULT		= 0x0F,
		OFFSET		= 0x10
	};

	static const uchar BROADCAST = 0xff;
	static const uchar IdLng = 4;

	static QString GetMsgTypeName(uchar);
	static QString GetParamFieldName(uchar);
	static QString GetParamFieldTypeName(uchar);
	
	ProtosMessage();
	ProtosMessage(const QByteArray&);
	ProtosMessage(const QString&);
	ProtosMessage(uchar destAddr, uchar senderAddr, ProtosMessage::MsgTypes msgType, uchar dlc, uchar data0, uchar data1 = 0, uchar data2 = 0, uchar data3 = 0, uchar data4 = 0, uchar data5 = 0, uchar data6 = 0, uchar data7 = 0, uchar data8 = 0);	
	ProtosMessage(uchar destAddr, uchar senderAddr, uchar paramId, ProtosMessage::MsgTypes msgType, ProtosMessage::ParamFields paramField = VALUE, QVariant fieldValue = QVariant());
	ProtosMessage(uchar destAddr, uchar senderAddr, uchar paramId, char  charValue,	 ProtosMessage::MsgTypes msgType = PSET);
	ProtosMessage(uchar destAddr, uchar senderAddr, uchar paramId, short shortValue, ProtosMessage::MsgTypes msgType = PSET);
	ProtosMessage(uchar destAddr, uchar senderAddr, uchar paramId, int   intValue,	 ProtosMessage::MsgTypes msgType = PSET);
	ProtosMessage(uchar destAddr, uchar senderAddr, uchar paramId, float floatValue, ProtosMessage::MsgTypes msgType = PSET);
	ProtosMessage(uchar destAddr, uchar senderAddr, uchar paramId, const QString& stringValue, ProtosMessage::MsgTypes msgType = PSET);
	~ProtosMessage();

	void FromByteArray(const QByteArray&);
	bool FromString(const QString& msg);
	uchar GetCmdId() const;
	QString GetErrorMsg() const;
	QString GetMsgType() const;	
	uchar GetParamFieldLength() const;
	QVariant GetParamFieldValue() const;
	QString GetParamField() const;
	uchar GetParamId() const;
	virtual bool IsCorrect(QString& errorMsg) const;	
	
	ProtosMessage& SetCanIdFields(ProtosMessage::MsgTypes msgType, uchar destAddr = 0, uchar senderAddr = 0);
	ProtosMessage& SetParamField(ProtosMessage::ParamFields field, uchar paramId = 0, ProtosMessage::MsgTypes msgType = NONE, QVariant fieldValue = QVariant(), uchar destAddr = 0, uchar senderAddr = 0);
	ProtosMessage& SetParamValue(char  charValue,  uchar paramId = 0, ProtosMessage::MsgTypes msgType = PSET, uchar destAddr = 0, uchar senderAddr = 0);
	ProtosMessage& SetParamValue(short shortValue, uchar paramId = 0, ProtosMessage::MsgTypes msgType = PSET, uchar destAddr = 0, uchar senderAddr = 0);
    void SetParamValue(short, bool);
    void SetParamValue(float, bool);
    ProtosMessage& SetParamValue(int intValue,   uchar paramId = 0, ProtosMessage::MsgTypes msgType = PSET, uchar destAddr = 0, uchar senderAddr = 0);
	ProtosMessage& SetParamValue(float floatValue, uchar paramId = 0, ProtosMessage::MsgTypes msgType = PSET, uchar destAddr = 0, uchar senderAddr = 0);
	ProtosMessage& SetParamValue(const QString& stringValue, uchar paramId = 0, ProtosMessage::MsgTypes msgType = PSET, uchar destAddr = 0, uchar senderAddr = 0);

	QByteArray ToByteArray() const;
	QString ToString() const;
	
	uchar& operator[](const uchar& index);	

	union
	{
		uint  CanId;
		uchar IdBytes[4];
		struct
		{
			uchar MsgType2		: 4;	// IdBytes[0]
			uchar MsgType		: 4;	// IdBytes[0]
			uchar SenderAddr;			// IdBytes[1]
			uchar DestAddr;				// IdBytes[2]		
			uchar BootMsgType	: 3;	// IdBytes[3]
			uchar BootLoader    : 1;	// IdBytes[3]
			uchar ProtocolType	: 1;	// IdBytes[3]
		};
	};	
	union
	{		
		struct
		{
			ushort Dlc;					// Dlc = ushort � ���� � ��������� ��������� ���������, ����� ���������� ������������ �� 4-� �������� �������� ��� Data
			uchar  Data[8];
		};
		struct
		{
			ushort Dlc2;					// Dlc ������ 2 �����, �������� Data, ��� ����� Data[2] - Data[5] ������ �� ������� int, uint � �.�....
			union
			{
				uchar CmdId;			// Data[0]
				uchar ParamId;			// Data[0]
			};
			uchar ParamFieldType : 3;	// Data[1]
			uchar ParamField	 : 5;	// Data[1]
			union
			{
				char	CharField;		// Data[2]
				uchar	UcharField;		// Data[2]
				short	ShortField;		// Data[2] - LSB, Data[3] - MSB
				ushort	UshortField;	// Data[2] - LSB, Data[2] - MSB
				int		LongField;		// Data[2] - LSB, Data[3], Data[4], Data[5] - MSB
				uint	UlongField;		// Data[2] - LSB, Data[3], Data[4], Data[5] - MSB
				float	FloatField;		// Data[2] - LSB, Data[3], Data[4], Data[5] - MSB
			};
		};		
	};
	/*union
	{
		uchar Data[8];
		struct 
		{
			union
			{
				uchar CmdId;
				uchar ParamId;
			};			
			uchar ParamFieldType : 3;		
			uchar ParamField	 : 5;
			union
			{
				char	CharField;
				uchar	UcharField;
				short	ShortField;
				ushort	UshortField;
				ushort		LongField;
				ushort	UlongField;
				ushort	FloatField;
			};
		};
	};*/
    uchar GetSenderAddr() const;
    uchar GetDestAddr() const;
};

Q_DECLARE_METATYPE(ProtosMessage)

//int i = qRegisterMetaType<ProtosMessage>("ProtosMessage");

#endif // !PROTOS_MESSAGE_H
