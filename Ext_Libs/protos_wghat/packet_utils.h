#include <vector>

enum
{
	//device TO\FROM server
	PROTOS_PACKET_START_BYTE_TO_SERVER   = '*',
	PROTOS_PACKET_START_BYTE_FROM_SERVER = '^',
	CTRL_PACKET_START_BYTE_FROM_SERVER = '#',
	CTRL_PACKET_START_BYTE_TO_SERVER   = '!',
	PACKET_STOP_BYTE = '\n'
};

enum
{
	MAX_PACKET_2X_SIZE = 28
};

std::vector<char> MakeCTRLPacket(const std::vector<char>& ctrlCmd);
extern int Pack2x(const char* src, int len, char* dst);
extern std::vector<char> Pack2x(const char* src, int len);
extern std::vector<char> Pack2x(const std::vector<char>& src);
extern int Protos2xPacketToFixedPacket(const std::vector<char>& src, char* dst);
extern int FixedPacketTo2x(const std::vector<char>& src, char* dst);
extern int Unpack2x(const char* src, int len, char* dst, bool noDLC);
