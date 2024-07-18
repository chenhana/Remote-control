#pragma once
//����һ��������
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
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {  //���ڴ�һ�����̵ķ������ƣ�access token��
			ShowError();
			return false;
		}
		TOKEN_ELEVATION eve;
		DWORD len = 0;
		// ����ѯ����洢��eve����ָ��Ļ������С�
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
		//TODO ��ȡ����ԱȨ�ޣ�ʹ�ø�Ȩ�޴�������
		// ���ز����飬����Administrator�˻� ��ֹ������ֻ�ܵ�¼���ؿ���̨
		STARTUPINFO si = { 0 };
		PROCESS_INFORMATION pi = { 0 };
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		BOOL ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
		if (!ret) {
			ShowError();
			MessageBox(NULL, _T("��������ʧ��"), _T("�������"), 0);
			return false;
		}
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return true;
	}

	// �ú���ͨ������FormatMessage��������ȡ�����ϵͳ������Ϣ��
// Ȼ��ʹ��OutputDebugString������������Ϣ���������������ڡ����ʹ��LocalFree�����ͷŷ�����ڴ�ռ䡣
	static void ShowError() {
		LPWSTR lpMessageBuf = NULL;
		FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&lpMessageBuf, 0, NULL);
		OutputDebugString(lpMessageBuf);
		MessageBox(NULL, lpMessageBuf, _T("��������"), 0);
		LocalFree(lpMessageBuf);
	}

	static BOOL WriteStartupDir(const CString& strPath) {
		//ͨ���޸Ŀ��������ļ�����ʵ�ֿ�������
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		return CopyFile(sPath, strPath, FALSE);
		
	}

	// ����������ʱ�򣬳����Ȩ���Ǹ��������û���
	// �������Ȩ�޲�һ�£���ᵼ�³�������ʧ��
	// ���������Ի���������Ӱ�죬�������dll�����������ʧ��
	// ������Щdll��system32������64λ���򣩻��� sysWoW64λ����

	static bool WriteregisterTable(const CString& strPath) {
		//ͨ���޸�ע�����ʵ�ֿ�������
		CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
		TCHAR  sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);  //��ȡ��ǰ��ִ��ģ�飨exe �ļ������ļ�·�����洢��sPath��
		BOOL ret = CopyFile(sPath, strPath, FALSE);
		if (ret = FALSE) {
			MessageBox(NULL, _T("�����ļ�ʧ�ܣ��Ƿ�Ȩ�޲��㣿\r\n"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}

		//����һ���������ַ�����ʹ�� mklink �����һ�������ӣ��������Ƶ�ϵͳĿ¼�£�ʵ�ֿ�����������
		//std::string strCmd = "mklink " + std::string(sSys) + strExe + std::string(sPath) + strExe;
		HKEY hKey = NULL;
		ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);  //��ע�����������·��д��ע����Ա�ϵͳ����ʱ�Զ�����
		if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			MessageBox(NULL, _T("�����Զ���������ʧ�ܣ��Ƿ�Ȩ�޲��㣿"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
		if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			MessageBox(NULL, _T("�����Զ���������ʧ�ܣ��Ƿ�Ȩ�޲��㣿"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		RegCloseKey(hKey);
		return true;
	}

	static bool Init() {
		//���ڴ�mfc����������Ŀ��ʼ��
		HMODULE hModule = ::GetModuleHandle(nullptr); //���д���������ǻ�ȡ��ǰ���̵�ģ����
		if (hModule == nullptr) {
			wprintf(L"����: GetModuleHandle ʧ��\n");
			return false;
		}
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0)) {
			wprintf(L"����: MFC ��ʼ��ʧ��\n");
			return false;
		}
		return true;
	}

};

