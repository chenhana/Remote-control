﻿#pragma once
#ifndef WM_SEND_PACK_ACK
#define  WM_SEND_PACK_ACK (WM_USER + 2) //发送包数据应答
#endif

// CWatchDialog 对话框

class CWatchDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CWatchDialog)

public:
	CWatchDialog(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CWatchDialog();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_WATCH };
#endif

public:
	int m_nObjWidth;
	int m_nObjHeight;
	CImage m_image;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	bool m_isFull; //缓存是否有数据，true是有数据

	DECLARE_MESSAGE_MAP()
public:
	CImage& GetImage() {
		return m_image;
	}
	void SetImageStatus(bool isFull = false) {
		m_isFull = isFull;
	}
	bool isFull() const {  //const 表示不会修改任何成员变量的值
		return m_isFull;
	}
	CPoint UserPoint2RemoteScreenPoint(CPoint& point,bool isScreen = false);
	virtual BOOL OnInitDialog();
	afx_msg LPARAM OnSendPacketAck(WPARAM wParam, LPARAM lParam);
	//afx_msg void OnTimer(UINT_PTR nIDEvent);
	CStatic m_picture;
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnStnClickedWatch();
	virtual void OnOK();
	afx_msg void OnBnClickedBtnUnlock();
	afx_msg void OnBnClickedBtnLock();
};
