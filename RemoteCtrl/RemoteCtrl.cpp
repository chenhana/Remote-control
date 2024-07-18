// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"
#include "EdoyunTool.h"
#include <conio.h>
#include "CEdoyunQueue.h"
#include <MSWSock.h>
#include "EdoyunServer.h"
//#include <Windows.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 唯一的应用程序对象
//#define  INVOKE_PATH _T("C:\\Windows\\SysWOW64\\Remote Ctrl.exe")
#define  INVOKE_PATH _T("C:\\Users\\Administrator\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe")
CWinApp theApp;
using namespace std;


bool ChooseAutoInvoke(const CString& strPath) {
	TCHAR wcsSystem[MAX_PATH] = _T("");
	if (PathFileExists(strPath)) {
		return true;
	}
	CString strInfo = _T("该程序只允许用于合法的用途! \n");
	strInfo += _T("继续运行该程序，将使得这台机器处于监控状态! \n");
	strInfo += _T("如果你不希望这样，请按“取消”按钮 ，退出程序\n");
	strInfo += _T("按下“是”按钮，该程序将被复制到你的机器上，并随系统启动而自动运行\n");
	strInfo += _T("按下“否”按钮，程序只运行一次，不会在系统内留下任何东西\n");
	int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
	if (ret == IDYES) {
		//WriteregisterTable(strPath);
		if(!CEdoyunTool::WriteStartupDir(strPath)){
			MessageBox(NULL, _T("复制文件失败，是否权限不足？\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
	}
	else if (ret == IDCANCEL) {
		return false;
	}
	return true;
}

void iocp(); 

void udp_server();
void udp_client(bool ishost = true);
void initsock() {
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
}
void clearsock() {
	WSACleanup();
}

int main(int argc, char* argv[])
{
	
	//应用程序通过创建一个 I/O 完成端口对象，并将该端口与相关的文件句柄或套接字关联起来。当一个 I/O 操作完成时，操作系统会将一个完成包投递到 I/O 完成端口上，应用程序可以通过调用 GetQueuedCompletionStatus 函数从 I/O 完成端口获取已完成的 I/O 操作和相应的数据。通过这种方式，应用程序可以避免阻塞式地等待 I/O 操作完成，从而提高程序的吞吐量和响应性能。
	if (!CEdoyunTool::Init()) return 1;
	initsock();
	if (argc == 1) {
		char wstrDir[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, wstrDir);
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		memset(&si, 0, sizeof(si));
		memset(&pi, 0, sizeof(pi));
		string strCmd = argv[0];
		strCmd += " 1";
		BOOL bRet = CreateProcessA(NULL,(LPSTR)strCmd.c_str(),NULL,NULL, FALSE,0,NULL,wstrDir,&si,&pi);
		if (bRet) {
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
			TRACE("进程ID:%d\r\n",pi.dwProcessId);
			TRACE("线程ID:%d\r\n", pi.dwThreadId);
			strCmd += "2";
			bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
			if (bRet) {
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
				TRACE("进程ID:%d\r\n", pi.dwProcessId);
				TRACE("线程ID:%d\r\n", pi.dwThreadId);
				udp_server();
			}

		}
	}
	else if(argc == 2) // 主客户端
	{
		udp_client();
	}
	else  // 从客户端
	{
		udp_client(false);
	}
	clearsock();
	//iocp();
	/*
	if (CEdoyunTool::IsAdmin()) {
		if (!CEdoyunTool::Init()) return 1;
		OutputDebugString(L"current is run as administrator! \r\n");
		if (!ChooseAutoInvoke(INVOKE_PATH)) {
			CCommand cmd;
			//Run 中是网络层 ，通过回调函数将业务层的封装到RunCommand
			int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);
			switch (ret) {
			case -1:
				MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
				break;
			case -2:
				MessageBox(NULL, _T("多次无法正常接入用户，结束程序"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
				break;
			}
		}
	}
	else
	{
		OutputDebugString(L"current is run as normal user! \r\n");
		if (CEdoyunTool::RunAsAdmin() == false) {
			CEdoyunTool::ShowError();
			return 1;
		}
	}*/
	return 0;
}

class COverlapped {
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;
	char m_buffer[4096];
	COverlapped() {
		m_operator = 0;
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		memset(&m_buffer, 0, sizeof(m_buffer));
	}
};

void iocp() {
	EdoyunServer server;
	server.StartServer();
	getchar();
	
}



void udp_server()
{
	printf("%s(%d):%s\r\n",__FILE__,__LINE__,__FUNCTION__);
	SOCKET sock = socket(PF_INET ,SOCK_DGRAM,0);
	if (sock != INVALID_SOCKET) {
		printf("%s(%d):%s ERROR\r\n", __FILE__, __LINE__, __FUNCTION__);
		return;
	}
	std::list<sockaddr_in> lstclients;
	sockaddr_in server, client;
	memset(&server, 0, sizeof(server));
	memset(&client, 0, sizeof(client));
	server.sin_family = AF_INET;
	server.sin_port = htons(20000);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	 
	if (::bind(sock, (sockaddr*)&server, sizeof(server))==-1)
	(-1 == bind(sock, (sockaddr*)&server, sizeof(server)))
	{
		printf("%s(%d):%s ERROR(%d)\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
		closesocket(sock);
		return;
	}
	std::string buf;
	buf.resize(1024 * 256);
	memset((char*)buf.c_str(), 0, buf.size());
	int len = sizeof(client);
	int ret = 0;
	while (!_kbhit)
	{
		ret = recvfrom(sock, (char*)buf.c_str(), buf.size(), 0, (sockaddr*)&client, &len);
		if (ret > 0) {
			if (lstclients.size() <= 0) {
				lstclients.push_back(client);
				printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, ntohs(client.sin_port));
				ret = sendto(sock, buf.c_str(), ret, 0, (sockaddr*)&client, len);
				printf("%s(%d):%s \r\n", __FILE__, __LINE__, __FUNCTION__);
			}
			else {
				memcpy((void*)buf.c_str(), &lstclients.front(), sizeof(lstclients.front()));
				ret = sendto(sock, buf.c_str(), sizeof(lstclients.front()), 0, (sockaddr*)&client, len);
				printf("%s(%d):%s \r\n", __FILE__, __LINE__, __FUNCTION__);
			}
			//CEdoyunTool::Dump((BYTE*)buf.c_str(), ret);

			
		}
		else
		{
			printf("%s(%d):%s ERROR(%d) !!! ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
		}
	}
	closesocket(sock);
	printf("%s(%d):%s ERROR\r\n", __FILE__, __LINE__, __FUNCTION__);
}

void udp_client(bool ishost)
{
	Sleep(2000);
	sockaddr_in server, client;
	int len = 0;
	server.sin_family = AF_INET;
	server.sin_port = htons(20000);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock != INVALID_SOCKET) {
		printf("%s(%d):%s ERROR\r\n", __FILE__, __LINE__, __FUNCTION__);
		return;
	}
	if (ishost) { //主客户端
		printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
		std::string msg = "hello world!\n";
		int ret = sendto(sock, msg.c_str(),msg.size(),0,(sockaddr*)&server, sizeof(server));
		printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__,ret);
		if (ret > 0) {
			msg.resize(1024);
			memset((char*)msg.c_str(), 0, msg.size());
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
			printf("host %s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
			if (ret > 0) {
				printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, ntohs(client.sin_port));
				printf("%s(%d):%s msg = %d\r\n", __FILE__, __LINE__, __FUNCTION__, msg.size());
			}
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
			printf("host %s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
			if (ret > 0) {
				printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, ntohs(client.sin_port));
				printf("%s(%d):%s msg = %d\r\n", __FILE__, __LINE__, __FUNCTION__, msg.size());
			}
		}
	}
	else //从客户端
	{
		printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
		std::string msg = "hello world!\n";
		int ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server, sizeof(server));
		printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
		if (ret > 0) {
			msg.resize(1024);
			memset((char*)msg.c_str(), 0, msg.size());
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
			printf("client %s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
			if (ret > 0) {
				sockaddr_in addr;
				memcpy(&addr, msg.c_str(), sizeof(addr));
				sockaddr_in* paddr = (sockaddr_in*)&addr;
				printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, client.sin_port);
				printf("%s(%d):%s msg = %d\r\n", __FILE__, __LINE__, __FUNCTION__, msg.size());
				printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, paddr->sin_addr.s_addr, ntohs(paddr->sin_port));
				msg = "hello , i am client!\r\n";
				ret = sendto(sock,(char*)msg.c_str(),msg.size(),0,(sockaddr*)paddr,sizeof(sockaddr_in));
				printf("client %s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
			}
		}
	}
	closesocket(sock);
}

