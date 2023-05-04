#pragma once

#include <vector>

struct FixedMsg
{
	std::vector<char> ToBinary() const
	{
		std::vector<char> bin;
		return ToBinary(bin);
	}
	std::vector<char>& ToBinary(std::vector<char>& bin) const
	{
		bin.resize(0);
		bin.reserve(Data.size() + 1);
		bin.push_back(Type);
		bin.insert(std::end(bin), Data.begin(), Data.end());
		return bin;
	}
	char Type;
	std::vector<char> Data;
};

struct FixedPacket
{
	FixedPacket() { ID.Word = 0; }

	FixedPacket(const QByteArray& data)
	{
		ID.Word = 0;
		AppendData(data);
	}

	FixedPacket(const char* data)
	{
		ID.Word = 0;
		AppendData(data);
	}

	FixedPacket(const char* data, int len)
	{
		ID.Word = 0;
		AppendData(data, len);
	}

	std::vector<char> ToBinary(char prefix) const
	{
		std::vector<char> bin;
		return ToBinary(prefix, bin);
	}

	std::vector<char>& ToBinary(char prefix, std::vector<char>& bin) const
	{
		bin.clear();
		bin.reserve(2 + Data.size());
		if (prefix) bin.push_back(prefix);
		bin.push_back((char)(ID.Word>>8));
		bin.push_back((char) ID.Word);
		bin.insert(std::end(bin), Data.begin(), Data.end());
		if (prefix) bin.push_back('\n');
		return bin;
	}

	FixedPacket& AppendData(const char* str)
	{
		return AppendData(str, strlen(str));
	}

	FixedPacket& AppendData(const std::vector<char>& data)
	{
		return AppendData(data.data(), data.size());
	}

	FixedPacket& AppendData(const QByteArray& data)
	{
		return AppendData(data.data(), data.size());
	}

	FixedPacket& AppendData(const char* buffer, int len)
	{
		if (ID.Bit.Len + len > 0x03FFF)
			return *this;
		Data.insert(std::end(Data), buffer, buffer + len);
		ID.Bit.Len += len;
		return *this;
	}

	union {
		struct
		{
			unsigned short Len  : 14;	// Data length
			unsigned short Type : 2;	// Type of Data content	
		}
		Bit;
		unsigned short Word = 0;
	} ID;
	std::vector<char> Data;
};

class FixedPacketView
{
public:
	FixedPacketView(const char* packet, int size)
		: Packet(packet)
		, Size(size)
	{}
	char Prefix()  const 
	{ 
		return Packet[0]; 
	}
	size_t Dlc()   const
	{
		char dlc[2] = { Packet[2], Packet[1]};
		return (*(short*)(dlc) & 0x3FFF);
	}
	const char* Data() const 
	{ 
		return &Packet[3];  
	}
private:
	const char* Packet;
	int Size;
};