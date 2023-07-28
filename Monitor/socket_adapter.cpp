#include "socket_adapter.h"
#include "mutex"
#include "QThread"

//Q_DECLARE_METATYPE(QAbstractSocket::SocketError)

SocketAdapter::SocketAdapter(bool multiThread) :
	MultiThread(multiThread),
	WaitForByte(START_BYTE)
{
	TimeoutTimer.setInterval(10);
	if (!MultiThread)
	{
		connect(&Socket, SIGNAL(readyRead()), this, SLOT(on_Socket_readyRead()));
		connect(&Socket, SIGNAL(bytesWritten(qint64)), this, SLOT(on_Socket_bytesWritten(qint64)));
	}
	connect(&Socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(on_Socket_error(QAbstractSocket::SocketError)));
	connect(&Socket, SIGNAL(connected()), this, SLOT(on_Socket_connected()));
	connect(&Socket, SIGNAL(connected()), this, SIGNAL(connected()));
	connect(&Socket, SIGNAL(disconnected()), this, SIGNAL(disconnected()));
	connect(&TimeoutTimer, SIGNAL(timeout()), this, SLOT(on_TimeoutTimer_timeout()));

	qRegisterMetaType <QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
}

SocketAdapter::~SocketAdapter()
{
}

void SocketAdapter::AddErrorHandler(const std::function<void(const QString&)>& errorHandler)
{
	ErrorHandlerList.append(errorHandler);
}

void SocketAdapter::AddRxMsgHandler(const std::function<void(const ProtosMessage&)>& rxMsgHandler)
{
	RxMsgHandlerList.append(rxMsgHandler);
}

void SocketAdapter::AddTxMsgHandler(const std::function<void(const ProtosMessage&)>& txMsgHandler)
{
	TxMsgHandlerList.append(txMsgHandler);
}

bool SocketAdapter::Connect(const QString& Ip, const int& Port, int Timeout)
{
	if (IsConnected())
		return false;

	Socket.connectToHost(Ip, Port);	
	Socket.waitForConnected(Timeout);
	if (IsConnected())
	{
		if (MultiThread)
		{
			ReadEn = true;
			std::thread ReadingThread(&SocketAdapter::ReadingLoop, this);
			ReadingThread.detach();
			WriteEn = true;
			std::thread WritingThread(&SocketAdapter::WritingLoop, this);
			WritingThread.detach();
		}
		return true;
	}
	else
		return false;	
}

void SocketAdapter::Disconnect(int Timeout)
{
	if (MultiThread)
	{
		ReadEnMutex.lock();
		WriteEnMutex.lock();
		ReadEn = false;
		WriteEn = false;
		ReadEnMutex.unlock();
		WriteEnMutex.unlock();
		std::lock_guard<std::mutex> LockerSocketReadingMutex(SocketReadingMutex);
		std::lock_guard<std::mutex> LockerSocketWritingMutex(SocketWritingMutex);
	}	
	Socket.disconnectFromHost();
	Socket.waitForDisconnected(Timeout);
}

void SocketAdapter::ErrorHandler(const QString& errSt)
{
	emit error(errSt);
	for (std::function<void(const QString&)>& Handler : ErrorHandlerList)
		Handler(errSt);
}

QString SocketAdapter::GetIp()
{
	return IsConnected() ? Socket.peerName() : "";
}

int SocketAdapter::GetPort()
{
	return IsConnected() ? Socket.peerPort() : -1;
}

int SocketAdapter::GetMsgKey(const ProtosMessage& msg)
{
	int key = 0;
	switch (msg.MsgType)
	{
	case ProtosMessage::MsgTypes::MCMD:
	case ProtosMessage::MsgTypes::SCMD:
	case ProtosMessage::MsgTypes::MANS:
	case ProtosMessage::MsgTypes::SANS:
	case ProtosMessage::MsgTypes::MERR:
	case ProtosMessage::MsgTypes::SERR:
		key |= 10000;
		key |= (msg.CmdId << 8) & 0xFF;
		break;
	case ProtosMessage::MsgTypes::PREQ:
	case ProtosMessage::MsgTypes::PANS:
	case ProtosMessage::MsgTypes::PSET:
	case ProtosMessage::MsgTypes::PERR:
		key |= 20000;
		key |= (msg.ParamId << 8) & 0xFF;
		break;
	}
	return key;
}

bool SocketAdapter::IsConnected()
{		
	return Socket.state() == QAbstractSocket::ConnectedState ? true : false;
}

bool SocketAdapter::IsStatusByteValid(const char& MsgType, const char& MsgLeng)
{
	return 0 < MsgLeng && MsgLeng <= MaxProtosMsgLeng ?  true : false;
}

void SocketAdapter::on_Socket_bytesWritten(qint64 bytes)
{
	std::lock_guard<std::mutex> txMsgMutexLocker(TxMsgMutex);
	ProtosMessage& txProtosMessage = TxMsgList.front();
	if (BytesToSend + ServiceBytesCnt == bytes)
		TxMsgHandler(txProtosMessage);			
	else
		WriteErrorHandler();		
	
	TxMsgList.pop_front();
	if (!TxMsgList.empty())
        QTimer::singleShot(adapterDelay, [this](){WriteMsg();});
}

void SocketAdapter::on_Socket_connected()
{
	if (!MultiThread && !TxMsgList.empty())
		WriteMsg();
}

void SocketAdapter::on_Socket_error(QAbstractSocket::SocketError socketError)
{
	QString errSt;
	switch (socketError)
	{
	case QAbstractSocket::ConnectionRefusedError:	
		errSt = tr("The connection was refused by the peer (or timed out).");
		break;
	case QAbstractSocket::RemoteHostClosedError:
		errSt = tr("The remote host closed the connection. note_ that the client socket (i.e., this socket) will be closed after the remote close notification has been sent.");
		break;
	case QAbstractSocket::HostNotFoundError:	
		errSt = tr("The host address was not found.");
		break;
	case QAbstractSocket::SocketAccessError:	
		errSt = tr("The socket operation failed because the application lacked the required privileges.");
		break;
	case QAbstractSocket::SocketResourceError:	
		errSt = tr("The local system ran out of resources(e.g., too many sockets).");
		break;
	case QAbstractSocket::SocketTimeoutError:	
		errSt = tr("The socket operation timed out.");
		break;
	case QAbstractSocket::DatagramTooLargeError:	
		errSt = tr("The datagram was larger than the operating system's limit (which can be as low as 8192 bytes).");
		break;
	case QAbstractSocket::NetworkError:	
		errSt = tr("An error occurred with the network(e.g., the network cable was accidentally plugged out).");
		break;
	case QAbstractSocket::AddressInUseError:	
		errSt = tr("The address specified to QAbstractSocket::bind() is already in use and was set to be exclusive.");
		break;
	case QAbstractSocket::SocketAddressNotAvailableError:	
		errSt = tr("The address specified to QAbstractSocket::bind() does not belong to the host.");
		break;
	case QAbstractSocket::UnsupportedSocketOperationError:	
		errSt = tr("The requested socket operation is not supported by the local operating system(e.g., lack of IPv6 support).");
		break;
	case QAbstractSocket::ProxyAuthenticationRequiredError:	
		errSt = tr("The socket is using a proxy, and the proxy requires authentication.");
		break;
	case QAbstractSocket::SslHandshakeFailedError:	
		errSt = tr("The SSL / TLS handshake failed, so the connection was closed(only used in QSslSocket).");
		break;
	case QAbstractSocket::UnfinishedSocketOperationError:	
		errSt = tr("Used by QAbstractSocketEngine only, The last operation attempted has not finished yet(still in progress in the background).");
		break;
	case QAbstractSocket::ProxyConnectionRefusedError:	
		errSt = tr("Could not contact the proxy server because the connection to that server was denied");
		break;
	case QAbstractSocket::ProxyConnectionClosedError:	
		errSt = tr("The connection to the proxy server was closed unexpectedly(before the connection to the final peer was established)");
		break;
	case QAbstractSocket::ProxyConnectionTimeoutError:	
		errSt = tr("The connection to the proxy server timed out or the proxy server stopped responding in the authentication phase.");
		break;
	case QAbstractSocket::ProxyNotFoundError:	
		errSt = tr("The proxy address set with setProxy() (or the application proxy) was not found.");
		break;
	case QAbstractSocket::ProxyProtocolError:	
		errSt = tr("The connection negotiation with the proxy server failed, because the response from the proxy server could not be understood.");
		break;
	case QAbstractSocket::OperationError:	
		errSt = tr("An operation was attempted while the socket was in a state_ that did not permit it.");
		break;
	case QAbstractSocket::SslInternalError:	
		errSt = tr("The SSL library being used reported an internal error.This is probably the result of a bad installation or misconfiguration of the library.");
		break;
	case QAbstractSocket::SslInvalidUserDataError:	
		errSt = tr("Invalid data(certificate, key, cypher, etc.) was provided and its use resulted in an error in the SSL library.");
		break;
	case QAbstractSocket::TemporaryError:	
		errSt = tr("A temporary error occurred(e.g., operation would block and socket is non - blocking).");
		break;
	case QAbstractSocket::UnknownSocketError:	
		errSt = tr("An unidentified error occurred.");
		break;
	default:
		errSt = tr("An unidentified error occurred.");
	}	
	ErrorHandler(errSt);	
}

void SocketAdapter::on_Socket_readyRead()
{
	ReadMsg();
}

void SocketAdapter::on_TimeoutTimer_timeout()
{
	QMap<int, TimeoutHandler>::iterator p = TimeoutHandlerMap.begin();
	while (p != TimeoutHandlerMap.end())
	{
		if (p.value().Timeout -= 10 <= 0)
		{
			emit timeout(p.key());
			p.value().Handler();
			p = TimeoutHandlerMap.erase(p);
		}
		else p++;
	}
}

void SocketAdapter::ReadingLoop()
{
	std::lock_guard<std::mutex> SocketReadingMutexLocker(SocketReadingMutex);
	std::lock_guard<std::mutex> ReadEnMutexLocker(ReadEnMutex);
	while (ReadEn)
	{
		ReadEnMutex.unlock();		
		if (Socket.waitForReadyRead(100))
			ReadMsg();	
		else if (!IsConnected())
		{
			QString errSt = tr("Host Server disconnected... :(");
			WriteEn = false;
			ReadEn = false;				
			ErrorHandler(errSt);
		}
		ReadEnMutex.lock();
	}	
}

void SocketAdapter::ReadMsg()
{	
	char byte;
	QString errSt = "";
	while (Socket.read(&byte, 1) == 1)
	{
		switch (WaitForByte)
		{
		case START_BYTE:
			if (byte == '#')
				WaitForByte = FIRST_STATUS_BYTE;
			break;
		case FIRST_STATUS_BYTE:
			RxMsgType = byte & 0xc0;
			RxMsgLeng = byte & 0x3f;
			RxMsgLeng <<= 8;
			RxMsgLeng &= 0x3f00;
			WaitForByte = SECOND_STATUS_BYTE;
			break;
		case SECOND_STATUS_BYTE:
			WaitForByte = DATA_BYTE;
			RxMsgLeng |= byte;
			RxProtosMsg.Dlc = RxMsgLeng - ProtosMessage::IdLng;
			RxMsgIndex = 0;
			break;
		/*case STATUS_BYTE:
			RxMsgType = byte & 0xc0;
			RxMsgLeng = byte & 0x3f;
			if (IsStatusByteValid(RxMsgType, RxMsgLeng))
			{
				RxProtosMsg.Dlc = RxMsgLeng - ProtosMessage::IdLng;					
				RxMsgIndex = 0;
				WaitForByte = DATA_BYTE;
			}
			else
			{
				errSt = tr("Got invalid status byte: 0x%1").arg(byte, 2, 16, QChar('0'));
				errSt.append(" :(");					
				ErrorHandler(errSt);
				WaitForByte = START_BYTE;
			}
			break;*/
		case DATA_BYTE:
			RxProtosMsg[RxMsgIndex++] = byte;
			if (RxMsgIndex == RxMsgLeng)
				WaitForByte = STOP_BYTE;				
			break;
		case STOP_BYTE:
			if (byte == '\r' || byte == '\n')
			{					
				RxMsgHandler(RxProtosMsg);						
			}
			else 
			{
				errSt = tr("Got invalid stop byte: 0x%0 after data byte sequence: ").arg(byte, 2, 16, QChar('0'));					
				for (ushort byteIndex = 0; byteIndex < RxMsgIndex; byteIndex++)
					errSt += " 0x" + QString("%0").arg(RxProtosMsg[byteIndex], 2, 16, QChar('0'));
				errSt.append(" :(");					
				ErrorHandler(errSt);
			}
			WaitForByte = START_BYTE;
			break;
		}
	}
}

void SocketAdapter::RxMsgHandler(const ProtosMessage& msg)
{
	TimeoutHandlerMap.remove(GetMsgKey(msg));
	emit rxMsg(msg);
	for (std::function<void(const ProtosMessage&)>& Handler : RxMsgHandlerList)
		Handler(msg);
}


void SocketAdapter::SendMsg(const ProtosMessage& txProtosMsg, const std::function<void(void)>& timeoutHandler, const int& timeout)
{
	if (timeoutHandler)
		TimeoutHandlerMap.insert(GetMsgKey(txProtosMsg), TimeoutHandler(timeout, timeoutHandler));
	std::lock_guard<std::mutex> TxMsgMutexLocker(TxMsgMutex);
	TxMsgList.push_back(txProtosMsg);
	if (!MultiThread && TxMsgList.size() == 1)
		WriteMsg();
}

void SocketAdapter::TxMsgHandler(const ProtosMessage& msg)
{
	emit txMsg(msg);
	for (std::function<void(const ProtosMessage&)>& Handler : TxMsgHandlerList)
		Handler(msg);
}

void SocketAdapter::WriteErrorHandler()
{
	QString errSt;
	if (IsConnected())
		errSt = tr("Unable to send (%1 bytes) to Host Server... :(").arg(BytesToSend + ServiceBytesCnt);
	else
	{
		errSt = tr("Host Server disconnected... :(");
		WriteEn = false;
		ReadEn = false;
	}
	ErrorHandler(errSt);
}

void SocketAdapter::WritingLoop()
{
	std::lock_guard<std::mutex> LockerSocketWritingMutex(SocketWritingMutex);
	std::unique_lock<std::mutex> LockerWriteEnMutex(WriteEnMutex);	
	std::unique_lock<std::mutex> LockerTxMsgMutex(TxMsgMutex, std::try_to_lock);
	while (WriteEn)
	{
		LockerWriteEnMutex.unlock();
		LockerTxMsgMutex.lock();
		if (!TxMsgList.empty())
		{
			if (WriteMsg())
				if (Socket.waitForBytesWritten(100))				
					TxMsgHandler(TxMsgList.front());				
				else
					WriteErrorHandler();
			TxMsgList.pop_front();
		}
		LockerTxMsgMutex.unlock();
		LockerWriteEnMutex.lock();
	}
}

bool SocketAdapter::WriteMsg()
{
	ProtosMessage& txProtosMsg = TxMsgList.front();
	BytesToSend = txProtosMsg.Dlc + ProtosMessage::IdLng;
	if (BytesToSend <= MaxProtosMsgLeng)
	{
		QByteArray txFixedMsg(BytesToSend + ServiceBytesCnt, '0');		
		ushort fixedMsgIndex = 0;
		txFixedMsg[fixedMsgIndex++] = '#';
		//txFixedMsg[fixedMsgIndex++] = 0x40 + (0x3f & BytesToSend);
		txFixedMsg[fixedMsgIndex++] = 0x40 + (0x3f & (BytesToSend >> 8));
		txFixedMsg[fixedMsgIndex++] = BytesToSend & 0xff;
		for (int protosMsgIndex = 0; protosMsgIndex < BytesToSend; protosMsgIndex++)
			txFixedMsg[fixedMsgIndex++] = txProtosMsg[protosMsgIndex];
		txFixedMsg[fixedMsgIndex] = '\r';
		if (Socket.write(txFixedMsg) == BytesToSend + ServiceBytesCnt)
		{
			return true;
		}
		else
		{
			WriteErrorHandler();
			return false;
		}
	}
	else
	{		
		QString errSt = tr("Try to send message longer (%1 bytes) than maximum PROTOS message length (%2 bytes).").arg(BytesToSend).arg(MaxProtosMsgLeng);
		ErrorHandler(errSt);
		return false;
	}
}

void SocketAdapter::setDelay(int delay) {
    if(delay > 0 && delay < 1000)
        adapterDelay = delay;
    else
        qDebug() << "Incorrect delay value!";
}

int SocketAdapter::getDelay() const{
    return adapterDelay;
}

