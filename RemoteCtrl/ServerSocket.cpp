#include "pch.h"
#include "ServerSocket.h"



// CServerSocket server;

CServerSocket* CServerSocket::m_instance = NULL;
CServerSocket::CHelper CServerSocket::m_helper;   //调用构造函数
CServerSocket* pserver = CServerSocket::getInstance();
