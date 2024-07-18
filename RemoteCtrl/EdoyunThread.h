#pragma once
#include "pch.h"
#include <atomic>
#include <vector>
#include <Windows.h>
#include <mutex>

class ThreadFuncBase {};
typedef int (ThreadFuncBase::* FUNCTYPE)(); //һ����Ϊ FUNCTYPE �����ͱ��������������ʾһ��ָ�� ThreadFuncBase ���Ա������ָ�룬�ó�Ա����û�в���������һ���������͵�ֵ

class ThreadWorker { 
public:
	ThreadWorker() :thiz(NULL), func(NULL) {};
	ThreadWorker(void* obj, FUNCTYPE f):thiz((ThreadFuncBase*)obj), func(f) {}
	ThreadWorker(const ThreadWorker& worker) {
		thiz = worker.thiz;
		func = worker.func;
	}
	ThreadWorker& operator=(const ThreadWorker& worker) {
		if (this != &worker) {

		}
		return *this;
	}

	int operator()() { //����������������أ���������ú���һ��ʹ�ö������磬�����һ��ThreadWorker����worker��������������һ��ʹ������int result = worker();
		if (IsValid()) {
			return (thiz->*func)();  //����ͨ����Աָ�� func ���ô洢�� thiz ָ��ָ��Ķ����ϵĳ�Ա���������������������������Ч���򷵻� -1���������ʹ�������Ķ�����Ա�����һ�����á�
		}
		return -1;
	}
	bool IsValid() const {
		return (thiz != NULL) && (func != NULL);
	}

	ThreadFuncBase* thiz;
	int (ThreadFuncBase::* func)(); //��Ա����ָ��
};

class EdoyunThread
{
public:
	EdoyunThread() {
		m_hThread = NULL;
		m_bStatus = false;
	}
	~EdoyunThread() {
		Stop();
	}
	bool Start() {
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(&EdoyunThread::ThreadEntry, 0, this);
		if (!IsValid()) {
			m_bStatus = false;
		}
		return m_bStatus;
	}
	bool IsValid(){  //����true��ʾ��Ч������false ��ʾ�߳��쳣�����Ѿ���ֹ
		if (m_hThread == NULL || (m_hThread == INVALID_HANDLE_VALUE)) return false;
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
	}
	bool Stop() {
		if(m_bStatus ==false ) return true;
		m_bStatus = false;
		bool ret = WaitForSingleObject(m_hThread, 1000) == WAIT_OBJECT_0;
		if (ret == WAIT_TIMEOUT) {
			TerminateThread(m_hThread,-1);
		}
		UpdateWorker();
		return ret;
	}

	void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker()) {
		if (m_worker.load() != NULL && (m_worker.load() != &worker)) {
			::ThreadWorker* pWorker = m_worker.load();
			m_worker.store(NULL);
			delete pWorker;
		}
		if (m_worker.load() == &worker) return;
		if (!worker.IsValid()) {
			m_worker.store(NULL);
			return ;
		}
		::ThreadWorker* pWorker = new ::ThreadWorker(worker);
		TRACE("pWorker = %08X m_worker= %08X\r\n", pWorker,m_worker.load());
		m_worker.store(pWorker);
	}

	bool IsIdle() {
		if (m_worker.load() == NULL)return true;
		return !m_worker.load()->IsValid();
	}
private:
	virtual void ThreadWorker() {
		while (m_bStatus)
		{
			if (m_worker.load() == NULL) {
				Sleep(1);
				continue;
			}
			::ThreadWorker worker = *m_worker.load(); //load() ������ԭ�ӱ����ṩ�ķ���֮һ�����ڼ��ص�ǰ��ֵ�����ء�
			if (worker.IsValid()) {
				if (WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT) {
					int ret = worker();  // worker ����� operator() ���������
					if (ret != 0) {
						CString str;
						str.Format(_T("thread found warning code %d\r\n"), ret);
						OutputDebugString(str);
					}
					if (ret < 0) {
						::ThreadWorker* pWorker = m_worker.load();
						m_worker.store(NULL);  //��һ���µ� ::ThreadWorker ����洢��ԭ�ӱ��� m_worker �С�
						delete pWorker;
					}
				}
			}
			else {
				Sleep(1);
			}
		}
	}
	static void ThreadEntry(void* arg) {
		EdoyunThread* thiz = (EdoyunThread*)arg;
		if (thiz) {
			thiz->ThreadWorker();
		}
		_endthread();
	}
private:
	HANDLE m_hThread;
	bool m_bStatus; // false ��ʾ�߳̽�Ҫ�ر� ��true ��ʾ�߳���������
	std::atomic<::ThreadWorker*> m_worker; 
};

class EdoyunThreadPool {
public:
	EdoyunThreadPool(size_t size) {
		m_threads.resize(size);
		for (size_t i = 0; i<size;i++)
		{
			m_threads[i] = new EdoyunThread();
		}
	}
	EdoyunThreadPool(){}
	~EdoyunThreadPool(){
		Stop();
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			delete m_threads[i];
			m_threads[i] = NULL;
		}
		m_threads.clear();
	}
	bool Invoke() {
		bool ret = true;
		for (size_t i =0;i<m_threads.size();i++)
		{
			if (m_threads[i]->Start() == false) {
				ret = false;
				break;
			}
		}
		if (ret == false) {
			for (size_t i = 0; i < m_threads.size(); i++)
			{
				m_threads[i]->Stop();
			}
		}
		return ret;
	}
	void Stop() {
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			m_threads[i]->Stop();
		}
	}

	//����-1 ��ʾ����ʧ�ܣ������̶߳���æ �����ڵ���0 ����ʾ��n���߳����������������
	int DispatchWorker(const ThreadWorker& worker){
		int index = -1;
		m_lock.lock(); // ��ֹ���̷ַ߳�ʱ��ɵĳ�ͻ
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			if (m_threads[i]->IsIdle()) {
				m_threads[i]->UpdateWorker(worker);
				index = i;
				break;
			}
		}
		m_lock.unlock();
		return index;
	}

	bool CheckThreadValid(size_t index) {
		if (index < m_threads.size()) {
			return m_threads[index]->IsValid();
		}
		return false;
	}
protected:
	std::mutex m_lock;
	std::vector<EdoyunThread*> m_threads;
};