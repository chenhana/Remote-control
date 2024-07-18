#pragma once
//这是一个工具类
class CEdoyunTool
{
public:
	static void Dump(BYTE* pData, size_t nSize)
	{
		std::string strout;
		for (size_t i = 0; i < nSize; i++)
		{
			char buf[8] = "";
			if (i > 0 && (i % 16 == 0)) strout += "\n";
			snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
			strout += buf;
		}
		strout += "\n";
		OutputDebugStringA(strout.c_str());
	}
	static bool IsAdmin() {
		HANDLE hToken = NULL;
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {  //用于打开一个进程的访问令牌（access token）
			ShowError();
			return false;
		}
		TOKEN_ELEVATION eve;
		DWORD len = 0;
		// 将查询结果存储在eve参数指向的缓冲区中。
		if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == FALSE) {
			ShowError();
			return false;
		}
		CloseHandle(hToken);
		if (len == sizeof(eve)) {
			return eve.TokenIsElevated;
		}
		printf("length of token information is %d\r\n", len);
		return false;
	}
	static bool RunAsAdmin() {
		//TODO 获取管理员权限，使用该权限创建进程
		// 本地策略组，开启Administrator账户 禁止空密码只能登录本地控制台
		STARTUPINFO si = { 0 };
		PROCESS_INFORMATION pi = { 0 };
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		BOOL ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
		if (!ret) {
			ShowError();
			MessageBox(NULL, _T("创建进程失败"), _T("程序错误"), 0);
			return false;
		}
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return true;
	}

	// 该函数通过调用FormatMessage函数来获取最近的系统错误信息，
// 然后使用OutputDebugString函数将错误信息输出到调试输出窗口。最后，使用LocalFree函数释放分配的内存空间。
	static void ShowError() {
		LPWSTR lpMessageBuf = NULL;
		FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&lpMessageBuf, 0, NULL);
		OutputDebugString(lpMessageBuf);
		MessageBox(NULL, lpMessageBuf, _T("发生错误"), 0);
		LocalFree(lpMessageBuf);
	}

	static BOOL WriteStartupDir(const CString& strPath) {
		//通过修改开机启动文件夹来实现开机启动
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		return CopyFile(sPath, strPath, FALSE);
		
	}

	// 开机启动的时候，程序的权限是跟随启动用户的
	// 如果两者权限不一致，则会导致程序启动失败
	// 开机启动对环境变量有影响，如果依赖dll，则可能启动失败
	// 复制这些dll到system32（多是64位程序）或者 sysWoW64位下面

	static bool WriteregisterTable(const CString& strPath) {
		//通过修改注册表来实现开机启动
		CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
		TCHAR  sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);  //获取当前可执行模块（exe 文件）的文件路径并存储在sPath中
		BOOL ret = CopyFile(sPath, strPath, FALSE);
		if (ret = FALSE) {
			MessageBox(NULL, _T("复制文件失败，是否权限不足？\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}

		//构建一个命令行字符串，使用 mklink 命令创建一个软链接，将程序复制到系统目录下，实现开机自启动。
		//std::string strCmd = "mklink " + std::string(sSys) + strExe + std::string(sPath) + strExe;
		HKEY hKey = NULL;
		ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);  //打开注册表项，将程序的路径写入注册表，以便系统启动时自动运行
		if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足？"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
		if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足？"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		RegCloseKey(hKey);
		return true;
	}

	static bool Init() {
		//用于带mfc命令行下项目初始化
		HMODULE hModule = ::GetModuleHandle(nullptr); //这行代码的作用是获取当前进程的模块句柄
		if (hModule == nullptr) {
			wprintf(L"错误: GetModuleHandle 失败\n");
			return false;
		}
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0)) {
			wprintf(L"错误: MFC 初始化失败\n");
			return false;
		}
		return true;
	}

};

