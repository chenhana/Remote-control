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
		//�� 32 λϵͳ�ϣ�size_t �Ĵ�Сͨ��Ϊ 32 λ���� 4 �ֽڡ��� 64 λϵͳ�ϣ�size_t �Ĵ�Сͨ��Ϊ 64 λ���� 8 �ֽڡ�
		size_t i = 0;
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF) { // һ����WORD��С���޷���������ͨ��Ϊ 16 λ
				sHead = *(WORD*)(pData + i);
				i += 2; //��ֹ����
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) { //���ȡ����ơ���У��  �����ݿ��ܲ�ȫ���߰�ͷδ��ȫ�����ܵ�
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nSize) { //��δ��ȫ���ܵ����ͷ��أ�����ʧ��
			nSize = 0;
			return;
		}

		sCmd = *(WORD*)(pData + i); i += 2;
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4); //c_str()����һ��ָ���Կ��ַ���β���ַ������ָ��
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
	int Size() { //�����ݴ�С
		return nLength + 6;
	}
	const char* Data() { //�����ݴ�С
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
	WORD sHead; //��ͷ �̶�FEFF
	DWORD nLength; //�����ȣ��ӿ������ʼ����У�������
	WORD sCmd; //��������
	std::string strData; //������
	WORD sSum; //��У��
	std::string strOut;

};

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.x = 0;

	}
	WORD nAction;//������ƶ���˫��
	WORD nButton;//������Ҽ����м�
	POINT ptXY; //����
}MOUSEEV, *PMOUSEEV;

typedef struct  file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));

	}
	char szFileName[256]; //�ļ���
	BOOL IsDirectory; //�Ƿ�ΪĿ¼ 0 ��1 ��
	BOOL HasNext;
	BOOL IsInvalid;//�Ƿ���Ч

}FILEINFO, *PFILEINFO;