#include "framework.h"
#include <Windows.h>
#include <string>
#include <atlimage.h>
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
	static int Byte2Image(CImage& image, const std::string& strBuffer) {
		std::string pbuffer= strBuffer;
		BYTE* pData = (BYTE*)strBuffer.c_str();  //TODO 存入Image
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
		if (hMem == NULL) {
			TRACE("内存不足了 \r\n");
			Sleep(1);
			return -1;
		}
		IStream* pStream = NULL;
		HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream); //将一个全局内存块转换为 IStream 接口的流对象
		if (hRet == S_OK) {
			ULONG length = 0;
			pStream->Write(pData, strBuffer.size(), &length);
			LARGE_INTEGER bg = { 0 };
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);
			if ((HBITMAP)image != NULL) image.Destroy();
			image.Load(pStream);
		}
		return hRet;
	}
};

