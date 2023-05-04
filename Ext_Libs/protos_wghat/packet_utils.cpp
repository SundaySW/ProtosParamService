#include "packet_utils.h"
#include <vector>
#include <cstddef>

std::vector<char> MakeCTRLPacket(const std::vector<char>& ctrlCmd)
{
	std::vector<char> result;
	result.push_back(CTRL_PACKET_START_BYTE_FROM_SERVER);
	result.insert(std::end(result), std::begin(ctrlCmd), std::end(ctrlCmd));
	result.push_back(PACKET_STOP_BYTE);
	return result;
}

//std::vector<char> MakeFixedControlPacket(const std::vector<char>& data)
//{
//	std::vector<char> packet;
//	packet.push_back('!');
//	unsigned short status = ((unsigned short)data.size() & 0x3FFF);
//	// make status bit for ctrl msg;
//	packet.push_back((char)(status >> 8));
//	packet.push_back((char)status);
//	packet.insert(std::end(packet), std::begin(data), std::end(data));
//	packet.push_back('\n');
//	return packet;
//}

int Pack2x(const char* src, int len, char* dst)
{
	int result = 0;
	char c[2];
	for (size_t i = 0; i < len; i++)
	{
		c[0] = ((src[i] >> 0x4) & 0x0F);
		c[1] = (src[i] & 0x0F);
		for (int i = 0; i < 2; i++)
		{
			if (c[i] >= 0xA && c[i] <= 0xF)
				c[i] += '7';
			else if (c[i] >= 0 && c[i] <= 9)
				c[i] += '0';
			else
				return 0;
		}
		dst[result++] = c[0];
		dst[result++] = c[1];
	}
	return result;
}

std::vector<char> Pack2x(const char* src, int len)
{
	std::vector<char> result(64);
	len = Pack2x(src, len, result.data());
	if (len)
		result.resize(len);
	return result;
}

std::vector<char> Pack2x(const std::vector<char>& src)
{
	return Pack2x(src.data(), src.size());
}

int Unpack2x(const char* src, int len, char* dst, bool noDLC)
{
	if ((len % 2) != 0) return 0;

	char c[2];
	int n = 0;
	for (int i = 0; i < len; i += 2)
	{
		if (i == 8 && noDLC) continue;

		c[0] = src[i]; c[1] = src[i + 1];

		for (int k = 0; k < 2; k++)
		{
			if (c[k] >= 'A' && c[k] <= 'F')
				c[k] -= '7';
			else if (c[k] >= '0' && c[k] <= '9')
				c[k] -= '0';
			else
				return 0;
		}
		dst[n++] = ((c[0] << 0x4) & 0xF0) | (c[1] & 0x0F);
	}
	return n;
}

int Protos2xPacketToFixedPacket(const std::vector<char>& src, char* dst)
{
	dst[0] = '#';
	dst[1] = 0x40;
	int n = Unpack2x(src.data() + 1, src.size() - 2, dst + 2, true);
	if (n)
	{
		dst[1] |= n;
		n += 2;
		dst[n++] = PACKET_STOP_BYTE;
	}
	return n;
}

//void AssemblePacketFixed(PacketDesc& packet, char byte)
//{
//	switch (packet.WaitStatus)
//	{
//	case PacketDesc::PAYLOAD_BYTES:
//		packet.Data.push_back(byte);
//		packet.PayloadLen--;
//		if (!packet.PayloadLen)
//			packet.WaitStatus = PacketDesc::STOP_BYTE;
//		break;
//	case PacketDesc::START_BYTE:
//		if (byte == '#')
//		{
//			packet.Data.push_back('#');
//			packet.WaitStatus = PacketDesc::STATUS_BYTE;
//		}
//		break;
//	case PacketDesc::STOP_BYTE:
//		if (byte == '\r' || byte == '\n')
//		{
//			packet.Data.push_back(byte);
//			packet.Complete = true;
//		}
//		else
//		{
//			packet.Reset();
//		}
//		break;
//	case PacketDesc::STATUS_BYTE:
//		packet.PayloadLen = (byte & 0x3F);
//		if (packet.PayloadLen == 0 || packet.PayloadLen > PacketDesc::MAX_PAYLOAD_SIZE)
//		{
//			packet.Reset();
//		}
//		else
//		{
//			packet.Data.push_back(byte);
//			packet.WaitStatus = PacketDesc::PAYLOAD_BYTES;
//		}
//		break;
//	};
//}
//
//void AssemblePacket2x(PacketDesc& packet, char byte)
//{
//	// * - protos message
//	// ! - control(text) message
//	if (byte == '*' || byte == '!')
//	{
//		if (packet.Data.size())
//			packet.Reset();
//
//		packet.Data.push_back(byte);
//		packet.WaitStatus = PacketDesc::PAYLOAD_BYTES;
//	}
//	else if (byte == '\n' || byte == '\r')
//	{
//		if (packet.Data.size() > 1/*start byte*/)
//		{
//			packet.Data.push_back(byte);
//			packet.Complete = true;
//		}
//	}
//	else
//	{
//		if (packet.WaitStatus == PacketDesc::PAYLOAD_BYTES)
//			packet.Data.push_back(byte);
//	}
//}

int FixedPacketTo2x(const std::vector<char>& src, char* dst)
{
	int len = 0;
	if (src[1] & 0x40)
	{//protos message
		dst[len++] = PROTOS_PACKET_START_BYTE_FROM_SERVER;
		char dlc = (src[1] & 0x0F) - 4;
		if (dlc > 8 || ((dlc + 7) != src.size()))
			return 0;
		len += Pack2x(&src[2], 4, dst + len);
		len += Pack2x(&dlc, 1, dst + len);
		len += Pack2x(&src[6], dlc, dst + len);
		dst[len++] = PACKET_STOP_BYTE;
	}
	return len;
}

//QString Packet2xToText(const std::vector<char>& msg)
//{
//	QString result;
//	QTextStream s(&result);
//	try
//	{
//		int i = 0;
//		s << msg.at(i++);
//		for (; i < 9; i++)
//		{
//			s << msg.at(i);
//		}
//		s << "." << msg.at(i++);
//		s << msg.at(i++) << "."; //DLC
//
//		int size = (int)msg.size();
//		for (; i < size; i++)
//			s << msg.at(i);
//	}
//	catch (...)
//	{
//		result.clear();
//		for (auto& c : msg)
//			result.push_back(c);
//	}
//	return result;
//}
//
//QString FixedPacketToText(const std::vector<char>& packet)
//{
//	QString result;
//	QTextStream s(&result);
//
//	std::vector<char> p = Pack2x(packet);
//	try
//	{
//		int i = 0;
//		s << p.at(i++);
//		s << p.at(i++) << "."; //start byte;
//		s << p.at(i++);
//		s << p.at(i++) << "."; //status byte;
//		for (int j = 0; j < 4; j++)
//		{
//			s << p.at(i++);
//			s << p.at(i++);
//		}
//		s << ".";
//		int size = (int)p.size();
//		for (; i < size - 2;)
//		{
//			s << p.at(i++);
//			s << p.at(i++); //data
//		}
//		s << ".";
//		for (; i < size;)
//		{
//			s << p.at(i++);
//			s << p.at(i++); //stop byte
//		}
//	}
//	catch (...)
//	{
//		result.clear();
//		for (auto& c : p)
//			result.push_back(c);
//	}
//	return result;
//}
//
//QString& FormatPacket(int protocolType, const std::vector<char>& packet, QString& text)
//{
//	switch (protocolType)
//	{
//	case PROTOCOL_FIXED:
//		text = QStringLiteral("RX: ").append(FixedPacketToText(packet));
//	break;
//	case PROTOCOL_2X:
//		text = QString("RX: ").append(Packet2xToText(packet));
//	break;
//	default:
//		text = "RX: ?";
//	};
//	return text;
//}