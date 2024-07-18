#pragma once
#include "pch.h"
#include "framework.h"
#include <vector>
#include <list>
#include "Packet.h"
#pragma pack(push)
#pragma pack(1)




typedef void(*SOCK_CALLBACK)(void* arg , int status , std::list<CPacket>&lstPacket,CPacket& ); //这段代码定义了一个名为 SOCK_CALLBACK 的类型，它是一个指向函数的指针类型，该函数接受一个 void* 类型的参数，并返回一个 int 类型的值

class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		if (m_instance == NULL) //静态函数没有this指针，无法直接访问成员变量
		{
			m_instance = new CServerSocket();
		}
		return m_instance;
	}

	int Run(SOCK_CALLBACK callback, void* arg, short port = 9527) {
		bool ret = InitSocket(port);
		if (ret == false)return -1;
		std::list<CPacket> lstPackets;
		m_callback = callback;
		m_arg = arg;
		int count = 0;
		while (true)
		{
			if (AcceptClient() == false) {
				if (count >= 3) {
					return -2;
				}
				count++;
			}
			int ret = DealCommand();
			if(ret > 0)
			{
				m_callback(m_arg, ret,lstPackets, m_packet);  //status command  使用lstPackets将所需的包导出
				while(lstPackets.size() > 0)
				{
					Send(lstPackets.front());
					lstPackets.pop_front();
				}
			}
			CloseClient();
		}
	
		return 0;
	}

protected:
	bool InitSocket(short port) {

		if (m_sock == -1)
		{
			return FALSE;
		}
		sockaddr_in serv_adr, client_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY;
		serv_adr.sin_port = htons(port);
		//绑定
		if (-1 == bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr))) {
			return FALSE;
		}
		//监听
		if (-1 == listen(m_sock, 1))
		{
			return false;
		}

		return TRUE;
	}

	bool AcceptClient()
	{
		sockaddr_in client_adr;
		int cli_sz = sizeof(client_adr);
		m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
		TRACE("m_client = %d\r\n", m_client);
		if (m_client == -1)
		{
			return false;
		}
		return true;
		//recv(m_sock, buffer, sizeof(buffer), 0);
		//send(m_sock, buffer, sizeof(buffer), 0);
	}

#define BUFFER_SIZE 40960000
	int DealCommand() {
		if (m_client == -1) return -1;
		//char buffer[1024] = "";
		//char *buffer = new char[BUFFER_SIZE];
		char *buffer = m_buffer.data();
		if (buffer == NULL)
		{
			TRACE("内存不足 \r\n");
			return -2;
		}
		//memset(buffer, 0, BUFFER_SIZE);
		static size_t index = 0;
		while (true)
		{
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			if (((int)len <= 0) && ((int)index <= 0)) { return -1; }
			TRACE("recv11 %d\r\n", len);
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
		
		return -1;
	}

	bool Send(const char* pData, size_t nSize) {
		if (m_client == -1)return false;
		return send(m_client, pData, nSize, 0) > 0;
	}
	bool Send(CPacket& pack) {
		if (m_client == -1)return false;
		//Dump((BYTE*)pack.Data(), pack.Size());
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}
	
	void CloseClient() {
		if (m_client != INVALID_SOCKET) {
			closesocket(m_client);
			m_client = INVALID_SOCKET;
		}
	
	}
private:
	SOCK_CALLBACK m_callback;
	void* m_arg;
	std::vector<char> m_buffer;
	SOCKET m_sock;
	SOCKET m_client;
	CPacket m_packet;
	CServerSocket& operator=(const CServerSocket& ss) {

	}
	CServerSocket(const CServerSocket& ss) {
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}
	CServerSocket(){
		m_sock = INVALID_SOCKET;
		m_client = INVALID_SOCKET;
		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL, _T("无法初始化套接字环境"), _T("初始化错误"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
		m_buffer.resize(BUFFER_SIZE);
		memset(m_buffer.data(), 0, BUFFER_SIZE);
	}
	~CServerSocket() {
		closesocket(m_sock);
		WSACleanup();
	}

	BOOL InitSockEnv() {
		WSADATA data;  //WSADATA 结构体：用于存储 Winsock 的版本信息和其他相关信息。
		if (WSAStartup(MAKEWORD(2, 0), &data) != 0) {
			return FALSE;
		}
		 //TODO 返回值处理 WSAStartup 函数：用于初始化 Winsock 库。它接受两个参数：要使用的 Winsock 版本，以及一个指向 WSADATA 结构体的指针，用于接收初始化信息。
		return TRUE;
	}
	static void releaseInstance()
	{
		if (m_instance != NULL) //静态函数没有this指针，无法直接访问成员变量
		{
			CServerSocket *tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}
	static CServerSocket* m_instance;// 静态是必须得初始化，在cpp中初始化


	class CHelper
	{
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releaseInstance();
		}
	};

	static CHelper m_helper;
};

extern CServerSocket server;