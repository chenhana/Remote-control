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
				TRACE("执行命令失败:%d ret=%d\r\n",status, ret);
			}
		}
		else {
			MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败"), MB_OK | MB_ICONERROR);

		}
	}
protected:
	typedef int(CCommand::* CMDFUNC)(std::list<CPacket>&, CPacket& inPacket); //成员函数指针定义了一个名为CMDFUNC的类型，它是一个成员函数指针类型。具体来说，它指向CCommand类的成员函数，该成员函数接受一个std::list<CPacket>&类型的引用参数和一个CPacket&类型的引用参数
	std::map<int, CMDFUNC> m_mapFunction; //从命令号到功能映射
	CLockDialog dlg;
	unsigned threadid ;

protected:
	static unsigned _stdcall threadLockDlg(void* arg) {
		//在函数内部，首先将传入的参数arg强制转换为CCommand*类型，然后调用该对象的threadLockDlgMain()成员函数，
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
		//窗口置顶
		dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		//限制鼠标功能
		ShowCursor(false);
		//隐藏任务栏
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);
		//限制鼠标活动范围
		dlg.GetWindowRect(rect);
		rect.left = 0;
		rect.top = 0;
		rect.right = 1;
		rect.bottom = 1;
		ClipCursor(rect);


		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {  //只能拿到该线程的消息
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_KEYDOWN) {
				if (msg.wParam == 0x1B) { //按下Esc退出
					break;
				}

			}
		}
		ClipCursor(NULL);
		//恢复鼠标
		ShowCursor(TRUE);
		//恢复任务栏
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
		dlg.DestroyWindow();
	}

	int MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) { // 1:A 2:B 3:C
		std::string result;
		for (int i = 1; i <= 26; i++)
		{
			if (_chdrive(i) == 0)  //_chdrive函数接受一个整数参数，表示要切换到的驱动器号
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
	
		if (_chdir(strPath.c_str()) != 0) {  //当前的工作目录切换到指定的路径
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			OutputDebugString(_T("没有权限，访问目录！"));
			return -2;
		}
		//这个函数接受一个文件路径的字符串参数 filespec，并使用它来查找匹配该路径模式的第一个文件或目录。同时，它将关于找到的文件或目录的信息存储在 fileinfo 结构体中。一旦找到了匹配的文件或目录，_findfirst 函数会返回一个指向文件 / 目录的句柄。
		//需要注意的是，在使用完 _findfirst 和 _findnext 进行文件搜索后，应该使用 _findclose 来关闭文件搜索句柄，以释放相关资源。
		_finddata_t fdata; //结构体
		int hfind = 0;
		if ((hfind = _findfirst("*", &fdata)) == -1) {
			OutputDebugString(_T("没有找到任何文件！"));
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
		//发送信息到控制端
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
		return 0;
	}

	int RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);  //ShellExecute函数来执行一个外部程序或打开一个文件
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
		case 0: //左键
			nFlags = 1;
			break;
		case 1:  //右键
			nFlags = 2;
			break;
		case 2: //中键
			nFlags = 4;
			break;
		case 3: //没有按键
			nFlags = 8;
			break;
		}
		if (nFlags != 8) SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
		switch (mouse.nAction)
		{
		case 0: //单击
			nFlags |= 0x10;
			break;
		case 1:  //双击
			nFlags |= 0x20;
			break;
		case 2: //按下
			nFlags |= 0x40;
			break;
		case 3: //放开
			nFlags |= 0x80;
			break;
		default:
			break;
		}
		TRACE("mouse event:%08X x %d y %d \r\n", nFlags, mouse.ptXY.x, mouse.ptXY.y);
		switch (nFlags)
		{
		case 0x21: //左键双击
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;

		case 0x11: //左键单击
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;

		case 0x41: //左键按下
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x81: //左键放开
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x22: //右键双击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x12: //右键单击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;

		case 0x42: //右键按下
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x82: //右键放开
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x24:  //中键双击
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x14:  //中键单击
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x44:  //中键按下
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x84:  //中键放开
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x08:  //鼠标移动
			mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
			break;
		}
		lstPacket.push_back(CPacket(5, NULL, 0));
		return 0;
	}

	int SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		CImage screen; //GDI 使用GDI库来进行屏幕捕获，并使用CImage类来存储屏幕图片
		HDC hScreen = ::GetDC(NULL); //通过调用::GetDC(NULL)来获取屏幕的设备上下文
		int nBitperPixel = GetDeviceCaps(hScreen, BITSPIXEL);
		int nWidth = GetDeviceCaps(hScreen, HORZRES);
		int nHeight = GetDeviceCaps(hScreen, VERTRES);
		screen.Create(nWidth, nHeight, nBitperPixel); //创建一个与屏幕分辨率和颜色深度相匹配的CImage对象
		BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY); //使用BitBlt函数将屏幕的内容复制到CImage对象。这里似乎有一个硬编码的大小(1920, 1020)，这可能是希望捕获屏幕特定区域的大小。
		ReleaseDC(NULL, hScreen);  //释放设备上下文
		HGLOBAL hMen = GlobalAlloc(GMEM_MOVEABLE, 0); //它使用 GlobalAlloc 函数来分配了一个可移动的内存块（内存句柄类型为 HGLOBAL） ,但在这里使用 0 代表让系统动态确定分配多少内存
		IStream* pStream = NULL;
		HRESULT ret = CreateStreamOnHGlobal(hMen, TRUE, &pStream); //CreateStreamOnHGlobal创建一个全局内存的流对象pStream，用于保存图像数据
		if (ret == S_OK) {
			screen.Save(pStream, Gdiplus::ImageFormatPNG); //从内存到流
			LARGE_INTEGER bg = { 0 }; //重置流的位置到起始处。
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
		//mbstowcs(sPath, strPath.c_str(), strPath.size()); 中文容易乱码
		MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), sPath, sizeof(sPath) / sizeof(TCHAR));
		DeleteFile(sPath);
		lstPacket.push_back(CPacket(9, NULL, 0));
		return 0;
	}
};

