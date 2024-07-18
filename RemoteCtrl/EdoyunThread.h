#pragma once
#include "pch.h"
#include <atomic>
#include <vector>
#include <Windows.h>
#include <mutex>

class ThreadFuncBase {};
typedef int (ThreadFuncBase::* FUNCTYPE)(); //一个名为 FUNCTYPE 的类型别名。这个别名表示一个指向 ThreadFuncBase 类成员函数的指针，该成员函数没有参数并返回一个整数类型的值

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

	int operator()() { //函数调用运算符重载，允许像调用函数一样使用对象。例如，如果有一个ThreadWorker对象worker，可以像函数调用一样使用它：int result = worker();
		if (IsValid()) {
			return (thiz->*func)();  //它将通过成员指针 func 调用存储在 thiz 指针指向的对象上的成员函数，并返回其结果。如果对象无效，则返回 -1。这种设计使得这个类的对象可以被像函数一样调用。
		}
		return -1;
	}
	bool IsValid() const {
		return (thiz != NULL) && (func != NULL);
	}

	ThreadFuncBase* thiz;
	int (ThreadFuncBase::* func)(); //成员函数指针
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
	bool IsValid(){  //返回true表示有效，返回false 表示线程异常或者已经终止
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
			::ThreadWorker worker = *m_worker.load(); //load() 方法是原子变量提供的方法之一，用于加载当前的值并返回。
			if (worker.IsValid()) {
				if (WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT) {
					int ret = worker();  // worker 对象的 operator() 运算符重载
					if (ret != 0) {
						CString str;
						str.Format(_T("thread found warning code %d\r\n"), ret);
						OutputDebugString(str);
					}
					if (ret < 0) {
						::ThreadWorker* pWorker = m_worker.load();
						m_worker.store(NULL);  //将一个新的 ::ThreadWorker 对象存储到原子变量 m_worker 中。
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
	bool m_bStatus; // false 表示线程将要关闭 ，true 表示线程正在运行
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

	//返回-1 表示分配失败，所有线程都在忙 ，大于等于0 ，表示第n个线程来分配做这个事情
	int DispatchWorker(const ThreadWorker& worker){
		int index = -1;
		m_lock.lock(); // 防止多线程分发时造成的冲突
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