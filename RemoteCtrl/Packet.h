#pragma once
#include "pch.h"
#include "framework.h"
#include <vector>
#include <list>
#pragma pack(push)
#pragma pack(1)

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
	const char* Data() { //包数据大小
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
	std::string strOut;

};

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
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