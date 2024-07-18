#include "pch.h"
#include "ClientController.h"
#include "ClientSocket.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;
CClientController* CClientController::m_instance = NULL;
CClientController::CHelper CClientController::m_helper;

CClientController* CClientController::getInstance()
{
	if (m_instance == NULL) {
		m_instance = new CClientController();
		struct 
		{
			UINT nMsg;
			MSGFUNC func;
		}MsgFuncs[]=
		{
			//{WM_SEND_PACK,&CClientController::OnSendPack},
			//{WM_SEND_DATA,&CClientController::OnSendData},
			{WM_SHOW_STATUS,&CClientController::OnShowStatus},
			{WM_SHOW_WATCH,&CClientController::OnShowWatcher},
		    {(UINT)-1,NULL}
		};
		for (int i = 0; MsgFuncs[i].func != NULL; i++) {
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].nMsg, MsgFuncs[i].func));
		}
	}
	return m_instance;
}

int CClientController::InitController()
{
	//_beginthreadex �ķ��������� uintptr_t����ʾ�߳̾�����������������ڲ����̣߳���ȴ��̵߳Ľ����ȡ�
		//_beginthread �ķ��������� unsigned int����ʾ�̵߳�Ψһ��ʶ���������ʶ�������߳̾��������ֱ�����ڲ����̡߳�
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientController::threadEntry, this, 0, &m_nThreadID);
	m_statusDlg.Create(IDD_DLG_STATUS,&m_remoteDlg);
	return 0;
}

int CClientController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg;
	return m_remoteDlg.DoModal();
}


bool CClientController::SendCommandPacket(HWND hWnd,int nCmd, bool bAutoClose, 
	BYTE * pData, size_t nLength , WPARAM wParam )
{
	TRACE("cmd :%d %s start %lld \r\n", nCmd, __FUNCTION__,GetTickCount64());
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret =  pClient->SendPacket(hWnd , CPacket(nCmd, pData, nLength),bAutoClose ,wParam );
	return ret;
}



void CClientController::DownloadEnd()
{
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(_T("������ɣ���"), _T("���"));
}

int CClientController::DownFile(CString strPath)
{
	
	CFileDialog dlg(FALSE, NULL, strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, &m_remoteDlg);
	if (dlg.DoModal() == IDOK) {
		m_strRemote = strPath;
		m_strLoacal = dlg.GetPathName();
		FILE* pFile = fopen(m_strLoacal, "wb+");
		if (pFile == NULL) {
			AfxMessageBox("����û��Ȩ�ޱ�����ļ����ļ��޷�����");
			return -1;
		}
		SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
		//����̺߳��� ��������̣߳���Ϣ���޷�ѭ��������ȴ��ļ��������
	/*	m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this);
		if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT) {
			return -1;			}*/
		m_remoteDlg.BeginWaitCursor();
		m_statusDlg.m_info.SetWindowText(_T("��������ִ����")); //SetWindowText �������ڸ��¾�̬�ı��򡢱༭��ȿؼ����ı����ݡ�
		m_statusDlg.ShowWindow(SW_SHOW);			
		m_statusDlg.CenterWindow(&m_remoteDlg);
		m_statusDlg.SetActiveWindow(); //ͨ�����Ի�������Ϊ����ڣ�����������������ڵ�ǰ�棬���ҿ��Խ��ռ�����������������¼���
	}
	
	return 0;
}

void CClientController::StartWatchScreen()
{
	m_isClosed = false;
	//m_watchDlg.SetParent(&m_remoteDlg);
	m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreen, 0, this);
	m_watchDlg.DoModal();  //��һ�������˴����ĶԻ��� dlg ��ģ̬�Ի�����Ϣѭ����ֱ���Ի��򱻹ر�
	m_isClosed = true;
	WaitForSingleObject(m_hThreadWatch, 500);
}

void CClientController::threadWatchScrenn()
{
	Sleep(50);
	ULONGLONG nTick = GetTickCount64();
	while (!m_isClosed)
	{
	 
		if (m_watchDlg.isFull() == false)
		{
			 
			if (GetTickCount64() - nTick < 200)
			{
				Sleep(200 - DWORD(GetTickCount64() - nTick));
			}
			nTick = GetTickCount64();
 
			int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), 6,true,NULL,0);
			//TODO �����Ϣ��Ӧ���� WM_SENF_PACK_ACK ,���Ʒ���Ƶ��
			if(ret == 1) {
				
				TRACE("�ɹ���������ͼƬ���� \r\n");
			}
			else
			{
				TRACE("��ȡͼƬʧ�ܣ�ret = %d \r\n", ret);

			}
			Sleep(1);
		 
		}
		
	}
}

void CClientController::threadWatchScreen(void * arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadWatchScrenn();
	_endthread();
}



void CClientController::threadFunc()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_SEND_MESSAGE) {

			MSGINFO* pmsg = (MSGINFO*)msg.wParam;
			HANDLE hEvent = (HANDLE)msg.lParam;

			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(pmsg->msg.message);
			if (it != m_mapFunc.end())
			{
				pmsg->result = (this->*it->second)(pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam);
			}
			else
			{
				pmsg->result = -1;
			}
			SetEvent(hEvent); //ͬʱwait ������Ҫ��֪ͨ�̵߳��¼�
		}
		else
		{
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end())
			{
				(this->*it->second)(msg.message, msg.wParam, msg.lParam);
			}
		}
		
	}
}

unsigned __stdcall CClientController::threadEntry(void * arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadFunc();
	_endthreadex(0);
	return 0;
}


LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}


