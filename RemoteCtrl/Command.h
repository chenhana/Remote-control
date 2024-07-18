#pragma once
#include <map>
#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "Packet.h"
#include <direct.h>
#include <atlimage.h>
#include "EdoyunTool.h"
#include <io.h>
#include <list>
#include "LockDialog.h"

class CCommand
{
public:
	CCommand();
	~CCommand() {

	}
	int ExcuteCommand(int nCmd, std::list<CPacket>& lstPacket,CPacket& inPacket);
	static void RunCommand(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket) {
		CCommand* thiz = (CCommand*)arg;
		if (status > 0) {
			int ret = thiz->ExcuteCommand(status,lstPacket, inPacket);
			if (ret!= 0) {
				TRACE("ִ������ʧ��:%d ret=%d\r\n",status, ret);
			}
		}
		else {
			MessageBox(NULL, _T("�޷����������û����Զ�����"), _T("�����û�ʧ��"), MB_OK | MB_ICONERROR);

		}
	}
protected:
	typedef int(CCommand::* CMDFUNC)(std::list<CPacket>&, CPacket& inPacket); //��Ա����ָ�붨����һ����ΪCMDFUNC�����ͣ�����һ����Ա����ָ�����͡�������˵����ָ��CCommand��ĳ�Ա�������ó�Ա��������һ��std::list<CPacket>&���͵����ò�����һ��CPacket&���͵����ò���
	std::map<int, CMDFUNC> m_mapFunction; //������ŵ�����ӳ��
	CLockDialog dlg;
	unsigned threadid ;

protected:
	static unsigned _stdcall threadLockDlg(void* arg) {
		//�ں����ڲ������Ƚ�����Ĳ���argǿ��ת��ΪCCommand*���ͣ�Ȼ����øö����threadLockDlgMain()��Ա������
		CCommand* thiz = (CCommand*)arg;
		thiz->threadLockDlgMain();
		_endthreadex(0);
		return 0;
	}
	void threadLockDlgMain() {
		dlg.Create(IDD_DIALOG_INFO, NULL);
		dlg.ShowWindow(SW_SHOW);
		CRect rect;
		rect.left = 0;
		rect.top = 0;
		rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
		rect.bottom = GetSystemMetrics(SM_CXFULLSCREEN);
		rect.bottom = LONG(rect.bottom *1.1);
		dlg.MoveWindow(rect);
		CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
		if (pText) {
			CRect rtText;
			pText->GetWindowRect(rtText);
			int nwidth = rtText.Width();
			int x = (rect.right - nwidth) / 2;
			int nHeight = rtText.Height();
			int y = (rect.bottom - nHeight) / 2;
			pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
		}
		//�����ö�
		dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		//������깦��
		ShowCursor(false);
		//����������
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);
		//���������Χ
		dlg.GetWindowRect(rect);
		rect.left = 0;
		rect.top = 0;
		rect.right = 1;
		rect.bottom = 1;
		ClipCursor(rect);


		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {  //ֻ���õ����̵߳���Ϣ
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_KEYDOWN) {
				if (msg.wParam == 0x1B) { //����Esc�˳�
					break;
				}

			}
		}
		ClipCursor(NULL);
		//�ָ����
		ShowCursor(TRUE);
		//�ָ�������
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
		dlg.DestroyWindow();
	}

	int MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) { // 1:A 2:B 3:C
		std::string result;
		for (int i = 1; i <= 26; i++)
		{
			if (_chdrive(i) == 0)  //_chdrive��������һ��������������ʾҪ�л�������������
			{
				if (result.size() > 0)
				{
					result += ',';
				}
				result += 'A' + i - 1;
			}
		}
		lstPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));

		return 0;
	}

	int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		//std::list<FILEINFO> listFileInfo;
	
		if (_chdir(strPath.c_str()) != 0) {  //��ǰ�Ĺ���Ŀ¼�л���ָ����·��
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			OutputDebugString(_T("û��Ȩ�ޣ�����Ŀ¼��"));
			return -2;
		}
		//�����������һ���ļ�·�����ַ������� filespec����ʹ����������ƥ���·��ģʽ�ĵ�һ���ļ���Ŀ¼��ͬʱ�����������ҵ����ļ���Ŀ¼����Ϣ�洢�� fileinfo �ṹ���С�һ���ҵ���ƥ����ļ���Ŀ¼��_findfirst �����᷵��һ��ָ���ļ� / Ŀ¼�ľ����
		//��Ҫע����ǣ���ʹ���� _findfirst �� _findnext �����ļ�������Ӧ��ʹ�� _findclose ���ر��ļ�������������ͷ������Դ��
		_finddata_t fdata; //�ṹ��
		int hfind = 0;
		if ((hfind = _findfirst("*", &fdata)) == -1) {
			OutputDebugString(_T("û���ҵ��κ��ļ���"));
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			return -3;
		}
		int count = 0;
		do {
			FILEINFO finfo;
			finfo.IsDirectory = (fdata.attrib&_A_SUBDIR) != 0;
			memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
			TRACE("%s \r\n", (BYTE*)finfo.szFileName);
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			count++;
			//listFileInfo.push_back(finfo);
		} while (!_findnext(hfind, &fdata));
		TRACE("server count  = %d\r\n", count);
		//������Ϣ�����ƶ�
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
		return 0;
	}

	int RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);  //ShellExecute������ִ��һ���ⲿ������һ���ļ�
		lstPacket.push_back(CPacket(3, NULL, 0));
		return 0;

	}
	int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		long long data = 0;
		FILE *pFile = NULL;
		errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
		if (err != 0) {
			lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
			return -1;
		}
		if (pFile != NULL)
		{
			fseek(pFile, 0, SEEK_END);
			data = _ftelli64(pFile);
			lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
			fseek(pFile, 0, SEEK_SET);
			char buffer[1024] = "";
			size_t rlen = 0;
			do {
				rlen = fread(buffer, 1, 1024, pFile);
				lstPacket.push_back(CPacket(4, (BYTE*)buffer, rlen));
			} while (rlen >= 1024);

			fclose(pFile);
		}
		else {
			lstPacket.push_back(CPacket(4, NULL, 0));
		}
		
		return 0;
	}
	int mouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		MOUSEEV mouse;
		memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV));
		DWORD nFlags = 0;

		switch (mouse.nButton)
		{
		case 0: //���
			nFlags = 1;
			break;
		case 1:  //�Ҽ�
			nFlags = 2;
			break;
		case 2: //�м�
			nFlags = 4;
			break;
		case 3: //û�а���
			nFlags = 8;
			break;
		}
		if (nFlags != 8) SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
		switch (mouse.nAction)
		{
		case 0: //����
			nFlags |= 0x10;
			break;
		case 1:  //˫��
			nFlags |= 0x20;
			break;
		case 2: //����
			nFlags |= 0x40;
			break;
		case 3: //�ſ�
			nFlags |= 0x80;
			break;
		default:
			break;
		}
		TRACE("mouse event:%08X x %d y %d \r\n", nFlags, mouse.ptXY.x, mouse.ptXY.y);
		switch (nFlags)
		{
		case 0x21: //���˫��
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;

		case 0x11: //�������
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;

		case 0x41: //�������
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x81: //����ſ�
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x22: //�Ҽ�˫��
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x12: //�Ҽ�����
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;

		case 0x42: //�Ҽ�����
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x82: //�Ҽ��ſ�
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x24:  //�м�˫��
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x14:  //�м�����
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x44:  //�м�����
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x84:  //�м��ſ�
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x08:  //����ƶ�
			mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
			break;
		}
		lstPacket.push_back(CPacket(5, NULL, 0));
		return 0;
	}

	int SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		CImage screen; //GDI ʹ��GDI����������Ļ���񣬲�ʹ��CImage�����洢��ĻͼƬ
		HDC hScreen = ::GetDC(NULL); //ͨ������::GetDC(NULL)����ȡ��Ļ���豸������
		int nBitperPixel = GetDeviceCaps(hScreen, BITSPIXEL);
		int nWidth = GetDeviceCaps(hScreen, HORZRES);
		int nHeight = GetDeviceCaps(hScreen, VERTRES);
		screen.Create(nWidth, nHeight, nBitperPixel); //����һ������Ļ�ֱ��ʺ���ɫ�����ƥ���CImage����
		BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY); //ʹ��BitBlt��������Ļ�����ݸ��Ƶ�CImage���������ƺ���һ��Ӳ����Ĵ�С(1920, 1020)���������ϣ��������Ļ�ض�����Ĵ�С��
		ReleaseDC(NULL, hScreen);  //�ͷ��豸������
		HGLOBAL hMen = GlobalAlloc(GMEM_MOVEABLE, 0); //��ʹ�� GlobalAlloc ������������һ�����ƶ����ڴ�飨�ڴ�������Ϊ HGLOBAL�� ,��������ʹ�� 0 ������ϵͳ��̬ȷ����������ڴ�
		IStream* pStream = NULL;
		HRESULT ret = CreateStreamOnHGlobal(hMen, TRUE, &pStream); //CreateStreamOnHGlobal����һ��ȫ���ڴ��������pStream�����ڱ���ͼ������
		if (ret == S_OK) {
			screen.Save(pStream, Gdiplus::ImageFormatPNG); //���ڴ浽��
			LARGE_INTEGER bg = { 0 }; //��������λ�õ���ʼ����
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);
			PBYTE pData = (PBYTE)GlobalLock(hMen);
			SIZE_T nSize = GlobalSize(hMen);
			lstPacket.push_back(CPacket(6, pData, nSize));
			GlobalUnlock(hMen);

		}
		pStream->Release();
		GlobalFree(hMen);
		screen.ReleaseDC();
		return 0;
	}

	int LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket) {

		if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE))
		{
			//_beginthread(threadLockDlg, 0, NULL);
			_beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid);

		}
		//_beginthread(threadLockDlg, 0, NULL);
		lstPacket.push_back(CPacket(7, NULL, 0));
		return 0;
	}

	int UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		//dlg.SendMessage(WM_KEYDOWN, 0x41, 0x01E0001);
		PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0);
		lstPacket.push_back(CPacket(8, NULL, 0));
		return 0;
	}

	int TestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		lstPacket.push_back(CPacket(1981, NULL, 0));
		return 0;
	}
	int DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		TCHAR sPath[MAX_PATH] = _T("");
		//mbstowcs(sPath, strPath.c_str(), strPath.size()); ������������
		MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), sPath, sizeof(sPath) / sizeof(TCHAR));
		DeleteFile(sPath);
		lstPacket.push_back(CPacket(9, NULL, 0));
		return 0;
	}
};

