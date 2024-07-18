#pragma once
#include <atomic>
#include "pch.h"

template<class T>
class CEdoyunQueue
{ // 线程安全队列
public:
	enum {
		EQNone,
		EQPush,
		EQPop,
		EQSize,
		EQClear
	};
	typedef struct IocpParam
	{
		size_t nOperator; //操作
		T Data; // 数据
		HANDLE hEvent; //pop操作需要
		IocpParam(int op, const T* data, HANDLE hEve = NULL) {
			nOperator = op;
			Data = data;
			hEven = hEve;
		}
		IocpParam() {
			nOperator = EQNone;
		}

	}PPARAM; //Post Parameter 用于投递信息的结构体
	
public:
	CEdoyunQueue() {
		m_lock = false;
		m_hCompelatationPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
		m_hThread = INVALID_HANDLE_VALUE;
		if (m_hCompelatationPort != NULL) {
			m_hThread = (HANDLE)_beginthread(
				&CEdoyunQueue<T> threadEntry,
				0, m_hCompelatationPort);
		}
	}
	~CEdoyunQueue() {
		if (m_lock) return;
		m_lock = ture;
		HANDLE hTemp = m_hCompelatationPort;
		PostQueuedCompletionStatus(m_hCompelatationPort, 0, NULL, NULL);
		WaitForSingleObject(m_hThread, INFINITE);
		m_hCompelatationPort = NULL;
		CloseHandle(hTemp);
	}
	bool PushBack(const T& data) {
		if (m_lock ) return false;
		IocpParam *pParam = new IocpParam(EQPush, data);
		bool ret = PostQueuedCompletionStatus(m_hCompelatationPort, sizeof(PPARAM),(ULONG_PTR)pParam,NULL);
		if (ret == false) delete pParam;
		return ret;
	}
	bool PopFront(T& data) {
		HANDLE hEvent = CreateEvent(NULL ,TRUE ,FALSE ,NULL);
		IocpParam Param(EQPop, data , hEvent);
		if (m_lock) {
			if (hEvent) CloseHandle(hEvent);
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompelatationPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return false;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) {
			data = pParam->Data;
		}
		return ret;
	}
	size_t Size() {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(EQSize, T(), hEvent);
		if (m_lock) {
			if (hEvent) CloseHandle(hEvent);
			return -1;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompelatationPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return -1;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) {
			return Param.nOperator;
		}
		return -1;
	}
	void Clear() {
		if (m_lock) return false;
		IocpParam *pParam = new IocpParam(EQClear, T());
		bool ret = PostQueuedCompletionStatus(m_hCompelatationPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false) delete pParam;
		return ret;
	}
private:
	static void threadEntry(void* arg) {
		CEdoyunQueue<T>* thiz = (CEdoyunQueue<T>*)arg;
		thiz->threadMain();
		_endthread();
	}
	void threadMain() {
		DWORD dwTransferred = 0;
		PPARAM* pParam = NULL;
		ULONG_PTR Completionkey = 0;  //用于接收与已完成 I/O 操作关联的完成键。完成键通常是指向与 I/O 操作相关的数据结构的指针。
		OVERLAPPED* pOverlapped = NULL;
		while (GetQueuedCompletionStatus(
			m_hCompelatationPort,
			&dwTransferred,
			&Completionkey, 
			&pOverlapped, INFINITE))
		{
			if ((dwTransferred == 0) || (Completionkey == NULL)) {
				printf("thread is prepare to exit");
				break;
			}
			pParam = (PPARAM*)Completionkey;
			switch (pParam->nOperator)
			{
				case EQPush:
					m_lstData.push_back(pParam->strData);
					delete pParam;
					break;
				case EQPop:
					if (m_lstData.size() > 0) {
						pParam->Data = lstString.front();
						m_lstData.pop_front();
					}
					if (pParam->hEvent != NULL) 
						SetEvent(pParam->hEvent);
					break;
				case EQSize:
					pParam->nOperator = m_lstData.size();
					if (pParam->hEvent != NULL) 
						SetEvent(pParam->hEvent);
					break;
				case EQClear:
					m_lstData.clear();
					delete pParam;
					break;
				default:
					OutputDebugString("unknown operator !\r\n");
					break;
			}
		}
		CloseHandle(m_hCompelatationPort);
	}
private:
	std::list<T> m_lstData;
	HANDLE m_hCompelatationPort;
	HANDLE m_hThread;
	std::atomic<bool> m_lock; //对列正在析构

};

