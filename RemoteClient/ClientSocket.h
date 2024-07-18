#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#include <list>
#include <map>
#include <mutex>
#define  WM_SEND_PACK (WM_USER + 1) //发送包数据
#define  WM_SEND_PACK_ACK (WM_USER + 2) //发送包数据应答
#pragma pack(push)
#pragma pack(1)


void Dump(BYTE* pData, size_t nSize);
class CPacket
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear();
		}

		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}

	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	CPacket(const BYTE* pData, size_t& nSize) {
		 
		//在 32 位系统上，size_t 的大小通常为 32 位，即 4 字节。在 64 位系统上，size_t 的大小通常为 64 位，即 8 字节。
		size_t i = 0;
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF) { // 一个字WORD大小的无符号整数，通常为 16 位
				sHead = *(WORD*)(pData + i);
				i += 2; //防止特殊
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) { //长度、控制、和校验  包数据可能不全或者包头未能全部接受到
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nSize) { //包未完全接受到，就返回，解析失败
			nSize = 0;
			return;
		}

		sCmd = *(WORD*)(pData + i); i += 2;
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4); //c_str()返回一个指向以空字符结尾的字符数组的指针
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i); i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum)
		{
			nSize = i;
			return;
		}
		nSize = 0;
	}
	~CPacket() {}
	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) {
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}
	int Size() { //包数据大小
		return nLength + 6;
	}
	const char* Data(std::string& strOut) const{ //包数据大小
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}
public:
	WORD sHead; //包头 固定FEFF
	DWORD nLength; //包长度（从控制命令开始到和校验结束）
	WORD sCmd; //控制命令
	std::string strData; //包数据
	WORD sSum; //和校验
	//std::string strOut;

};
#pragma pack(pop)

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		ptXY.x = 0;
		nButton = -1;
		ptXY.x = 0;

	}
	WORD nAction;//点击、移动、双击
	WORD nButton;//左键，右键，中键
	POINT ptXY; //坐标
}MOUSEEV, *PMOUSEEV;


typedef struct  file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));

	}
	char szFileName[256]; //文件名
	BOOL IsDirectory; //是否为目录 0 否，1 是
	BOOL HasNext;
	BOOL IsInvalid;//是否有效

}FILEINFO, *PFILEINFO;

enum {
	CSM_AUTOCLOSE = 1, //CSM 自动关闭模式
};

typedef struct PacketDate
{
	std::string strData;
	UINT nMode;
	WPARAM wParam;
	PacketDate(const char* pData ,size_t nLen ,UINT mode , WPARAM nParam =0) {
		strData.resize(nLen);
		memcpy((BYTE*)strData.c_str(),pData, nLen);
		nMode = mode;
		wParam = nParam;
	}
	PacketDate(const PacketDate& data) {
		strData = data.strData;
		nMode = data.nMode;
		wParam = data.wParam;
	}
	PacketDate& operator=(const PacketDate& data) {
		if (this != &data) {
			strData = data.strData;
			nMode = data.nMode;
			wParam = data.wParam;
		}
		return *this;
	}

}PACKET_DATA;

std::string GetErrInfo(int wsaErrCode);

class CClientSocket
{
public:
	static CClientSocket* getInstance() {
		if (m_instance == NULL) //静态函数没有this指针，无法直接访问成员变量
		{
			m_instance = new CClientSocket();
		}
		return m_instance;
	}
	bool InitSocket();
	
#define BUFFER_SIZE 40960000
	int DealCommand() {
		if (m_sock == -1) return -1;
		//char buffer[1024] = "";
		char *buffer = m_buffer.data();
		static size_t index = 0;
		while (true)
		{
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);

			if (((int)len <= 0) && ((int)index <= 0)) { return -1; }
			//Dump((BYTE*)buffer, index);

			TRACE("**************len: %d index: %d", len,index);
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			TRACE("len: %d index: %d", len, index);
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}

	//bool SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed = true);
	bool CClientSocket::SendPacket(HWND hWndk, const CPacket & pack, bool isAutoClosed = true, WPARAM wParam = 0);

	bool GetFilePath(std::string& strPath) //将包数据中的信息拿出来
	{
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) {
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}
	bool GetMouseEvent(MOUSEEV& mouse) {
		if (m_packet.sCmd == 5) {
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true;
		}
		return false;
	}
	CPacket& GetPacket() {
		return m_packet;
	}
	void CloseSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
	void UpdateADDRESS(INT nIP, int nPort) {
		if (m_nIP != nIP || m_nPort != nPort)
		{
			m_nIP = nIP;
			m_nPort = nPort;
		}
	}
private:
	HANDLE m_eventInvoke; //启动事件
	UINT m_nThreadID;
	typedef void(CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam,
		LPARAM lParam);
	std::map<UINT, MSGFUNC> m_mapFunc;
	HANDLE m_hThread;
	bool m_bAutoClose;
	std::mutex m_lock;
	std::list<CPacket> m_lstSend;
	std::map<HANDLE, std::list<CPacket>&> m_mapAck; //应答可能会有多个包，所以使用list
	std::map<HANDLE, bool> m_mapAutoClosed;
	int m_nIP;
	int m_nPort;
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet;
	CClientSocket& operator=(const CClientSocket& ss) {	}
	CClientSocket(const CClientSocket& ss);
	
	CClientSocket();
	~CClientSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		WSACleanup();
	}
	static unsigned __stdcall threadEntry(void* arg);
	//void threadFunc();
	void threadFunc2();
	BOOL InitSockEnv() {
		WSADATA data;  //WSADATA 结构体：用于存储 Winsock 的版本信息和其他相关信息。
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		}
		//TODO 返回值处理 WSAStartup 函数：用于初始化 Winsock 库。它接受两个参数：要使用的 Winsock 版本，以及一个指向 WSADATA 结构体的指针，用于接收初始化信息。
		return TRUE;
	}
	static void releaseInstance()
	{
		if (m_instance != NULL) //静态函数没有this指针，无法直接访问成员变量
		{
			CClientSocket *tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}

	bool Send(const char* pData, size_t nSize) {
		if (m_sock == -1)return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}
	bool Send(const CPacket& pack);
	void SendPack(UINT nMsg, WPARAM wParam/*缓冲区的值*/, LPARAM lParam /*缓存区的长度*/);
	static CClientSocket* m_instance;// 静态是必须得初始化，在cpp中初始化


	class CHelper
	{
	public:
		CHelper() {
			CClientSocket::getInstance();
		}
		~CHelper() {
			CClientSocket::releaseInstance();
		}
	};

	static CHelper m_helper;
};

extern CClientSocket server;