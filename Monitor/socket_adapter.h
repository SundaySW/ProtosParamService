#ifndef SOCKET_ADAPTER_H
#define SOCKET_ADAPTER_H

#include "protos_message.h"

#include <functional>
#include <mutex>
#include <thread>

#include <QString>
#include <QTcpSocket>
#include <QTime>
#include <QTimer>
#include <QVector>

//Q_DECLARE_METATYPE(QAbstractSocket::SocketError)

class SocketAdapter : public QObject
{
	Q_OBJECT

public:	
	SocketAdapter(bool multiThread = false);
	~SocketAdapter();

	void AddErrorHandler(const std::function<void(const QString&)>& errorHandler);
	void AddRxMsgHandler(const std::function<void(const ProtosMessage&)>& rxMsgHandler);
	void AddTxMsgHandler(const std::function<void(const ProtosMessage&)>& txMsgHandler);
	bool Connect(const QString& Ip, const int& Port, int Timeout = 1);
	void Disconnect(int Timeout = 0);
	QString GetIp();
	int  GetPort();
	bool IsConnected();	
	void SendMsg(const ProtosMessage& msg, const std::function<void(void)>& timeoutHandler = nullptr, const int& timeout = 0);
    void setDelay(int delay);
    int getDelay() const;

private:
	void ErrorHandler(const QString&);
	int  GetMsgKey(const ProtosMessage& msg);
	bool IsStatusByteValid(const char& MsgType, const char& MsgLeng);
	void ReadingLoop();
	void ReadMsg();
	void RxMsgHandler(const ProtosMessage&);
	void TxMsgHandler(const ProtosMessage&);
	void WriteErrorHandler();
	bool WriteMsg();
	void WritingLoop();
	
private slots:
	void on_Socket_bytesWritten(qint64 bytes);
	void on_Socket_connected();
	void on_Socket_error(QAbstractSocket::SocketError);
	void on_Socket_readyRead();
	void on_TimeoutTimer_timeout();

private:	
	int adapterDelay = 5;
	enum
	{
		START_BYTE,
		FIRST_STATUS_BYTE,
		SECOND_STATUS_BYTE,
		DATA_BYTE,
		STOP_BYTE
	};
	QTcpSocket Socket;
	ProtosMessage RxProtosMsg;
	QList<ProtosMessage> TxMsgList;
	QList<std::function<void(const QString&)>> ErrorHandlerList;
	QList<std::function<void(const ProtosMessage&)>> RxMsgHandlerList;
	QList<std::function<void(const ProtosMessage&)>> TxMsgHandlerList;
	struct TimeoutHandler
	{
		TimeoutHandler(const int& timeout, const std::function<void(void)>& handler) : Timeout(timeout), Handler(handler) {};
		int Timeout;
		std::function<void(void)> Handler;
	};
	QMap<int, TimeoutHandler> TimeoutHandlerMap;
	QTimer TimeoutTimer;
	const int ServiceBytesCnt = 4/*3*/, MaxProtosMsgLeng = 15;
	int WaitForByte, RxMsgType, RxMsgLeng, RxMsgIndex, BytesToSend;
	bool MultiThread, ReadEn, WriteEn;
	std::mutex ReadEnMutex, WriteEnMutex, SocketReadingMutex, SocketWritingMutex, TxMsgMutex;	

signals:
	void connected();
	void disconnected();
	void error(const QString&);	
	void rxMsg(const ProtosMessage&);
	void txMsg(const ProtosMessage&);
	void timeout(const int&);
};

#endif //SOCKET_ADAPTER_H