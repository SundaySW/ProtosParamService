#pragma once

#include <vector>

class PacketAssembler
{
public:
	enum PROTOCOL_TYPE
	{
		PROTOCOL_2X,
		PROTOCOL_FIXED,
	};
	enum { START_BYTE, STATUS_HIBYTE, STATUS_LOBYTE, PAYLOAD_BYTES, STOP_BYTE };
	
	PacketAssembler(PROTOCOL_TYPE protocol)
		: AssembleFunc(nullptr)
		, Protocol(protocol)
		, PayloadLen(0)
		, WaitStatus(START_BYTE)
		, Complete(false)
	{
		if (protocol == PROTOCOL_FIXED)
			AssembleFunc = &PacketAssembler::AssembleFixedPacket;
		else if (protocol == PROTOCOL_2X)
			AssembleFunc = &PacketAssembler::AssemblePacket2x;
	}

	void Assemble(char byte) { (this->*AssembleFunc)(byte); }
	bool IsCmd() const { return (Packet.size() && Packet.front() == '!');  };
	bool IsComplete() const { return Complete; }
	void Reset() { StatusWord[0] = StatusWord[1] = 0; WaitStatus = START_BYTE; Packet.clear(); Complete = false; }
	
	std::vector<char> Packet;

private:
	void AssembleFixedPacket(char byte)
	{
		switch (WaitStatus)
		{
		case PAYLOAD_BYTES:
			Packet.push_back(byte);
			PayloadLen--;
			if (!PayloadLen)
				WaitStatus = STOP_BYTE;
			break;
		case START_BYTE:
			if (byte == '#' || 
				byte == '!')//message from UI plugin
			{
				Packet.push_back(byte);
				WaitStatus = STATUS_HIBYTE;
			}
			break;
		case STOP_BYTE:
			if (byte == '\r' || byte == '\n')
			{
				Packet.push_back(byte);
				Complete = true;
			}
			else
			{
				Reset();
			}
			break;
		case STATUS_HIBYTE:
			StatusWord[1] = byte;
			Packet.push_back(byte);
			WaitStatus = STATUS_LOBYTE;
			break;
		case STATUS_LOBYTE:
			StatusWord[0] = byte;
			PayloadLen = ((*(short*)StatusWord) & 0x3FFF);
			if (PayloadLen == 0/* || PayloadLen > MAX_PAYLOAD_SIZE*/)
			{
				Reset();
			}
			else
			{
				Packet.push_back(byte);
				WaitStatus = PAYLOAD_BYTES;
			}
			break;
		};
	}
	void AssemblePacket2x(char byte)
	{
		// * - protos message
		// ! - control(text) message
		if (byte == '*' || byte == '!')
		{
			if (Packet.size())
				Packet.clear();

			Packet.push_back(byte);
			WaitStatus = PAYLOAD_BYTES;
		}
		else if (byte == '\n' || byte == '\r')
		{
			if (Packet.size() > 1/*start byte*/)
			{
				Packet.push_back(byte);
				Complete = true;
			}
		}
		else
		{
			if (WaitStatus == PAYLOAD_BYTES)
				Packet.push_back(byte);
		}
	}

	void (PacketAssembler::*AssembleFunc)(char);
	
	PROTOCOL_TYPE Protocol;
	int PayloadLen;
	int WaitStatus;
	char StatusWord[2];
	bool Complete;
};