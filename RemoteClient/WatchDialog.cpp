// WatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "WatchDialog.h"
#include "afxdialogex.h"
#include "ClientController.h"

// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialogEx)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_WATCH, pParent)
{
	m_isFull = false;
	m_nObjHeight = -1;
	m_nObjWidth = -1;
}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialogEx)
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDialog::OnBnClickedBtnUnlock)
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDialog::OnBnClickedBtnLock)
	ON_MESSAGE(WM_SEND_PACK_ACK,&CWatchDialog::OnSendPacketAck)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint & point, bool isScreen)
{
	CRect clientRect;
	if(!isScreen)  ClientToScreen(&point);  //屏幕内的绝对坐标
	m_picture.ScreenToClient(&point);  //相对picture控件左上角的坐标
	
	//本地坐标到远程坐标
	m_picture.GetWindowRect(clientRect); 
	return CPoint(point.x * m_nObjWidth / clientRect.Width(), point.y * m_nObjHeight / clientRect.Height());
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	m_isFull = false;
	//SetTimer(0, 45, NULL);  //通过调用 SetTimer() 函数设置了一个定时器，以便每隔50毫秒触发一次定时器消息。
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

LPARAM CWatchDialog::OnSendPacketAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || lParam == -2)
	{
		//TODO错误处理
	}
	else if (lParam == 1) {
		//对方关闭了套接字
	}
	else {
		CPacket* pPacket = (CPacket*)wParam;
		if (pPacket != NULL) {
			CPacket head = *(CPacket*)wParam;
			delete (CPacket*)wParam;
			switch (head.sCmd) {
			case 6:
			{
				CEdoyunTool::Byte2Image(m_image, head.strData);
				CRect rect;
				m_picture.GetWindowRect(rect);
				m_nObjHeight = m_image.GetHeight();
				m_nObjWidth = m_image.GetWidth();
				m_image.StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
				m_picture.InvalidateRect(NULL); //用于使指定窗口客户区的矩形区域失效，从而导致系统在接下来的消息循环中重新绘制该区域
				TRACE("更新图片完成%d %d %08X \r\n", m_nObjWidth, m_nObjHeight, (HBITMAP)m_image);
				m_image.Destroy();
				break;
			}

			case 5:
				TRACE("远程端应答鼠标操作\r\n");
			case 7:
			case 8:
			default:
				break;
			}
		}
		return 0;
	}
}


/*void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//if (nIDEvent == 0) {  //当定时器消息被触发时，会检查定时器的 nIDEvent 参数是否为0，如果是，则表示是刚刚设置的定时器被触发了
	//	
	//CClientController*  pParent = CClientController::getInstance();
	//	if (m_isFull) {
	//		CRect rect;
	//		m_picture.GetWindowRect(rect);
	//		m_nObjHeight =m_image.GetHeight();
	//		m_nObjWidth = m_image.GetWidth();
	//		//pParent->GetImage().BitBlt(m_picture.GetDC()->GetSafeHdc(),0,0,SRCCOPY);
	//		m_image.StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0,rect.Width(),rect.Height(), SRCCOPY);
	//		m_picture.InvalidateRect(NULL); //用于使指定窗口客户区的矩形区域失效，从而导致系统在接下来的消息循环中重新绘制该区域
	//		m_image.Destroy();
	//		m_isFull = false;
	//		TRACE("更新图片完成%d %d %08X \r\n", m_nObjWidth, m_nObjHeight,(HBITMAP)m_image);
	//	}
	//}
	CDialog::OnTimer(nIDEvent);
}*/


void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if ((m_nObjHeight != -1) && (m_nObjWidth != -1))
	{
		// 坐标转化
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		// 封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;  //左键
		event.nAction = 1; //双击
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),5, true,(BYTE*)&event ,sizeof(event));
	}
	CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	if ((m_nObjHeight != -1) && (m_nObjWidth != -1))
	{
		TRACE("x=%d y=%d \r\n", point.x, point.y);
		// 坐标转化
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		TRACE("x=%d y=%d \r\n", point.x, point.y);
		// 封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;  //左键
		event.nAction = 2; //弹起
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),5, true, (BYTE*)&event, sizeof(event));
		}
	CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
		// 坐标转化
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		// 封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;  //左键
		event.nAction = 3; //弹起
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),5, true, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
		// 坐标转化
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		// 封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;  //右键
		event.nAction = 1; //双击
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd() ,5, true, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
		// 坐标转化
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		// 封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;  //右键
		event.nAction = 2; //按下 服务器做对应修改
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd() ,5, true, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
		// 坐标转化
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		// 封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;  //左键
		event.nAction = 3; //弹起
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
		// 坐标转化
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		// 封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 8;  //移动
		event.nAction = 0; //移动
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()
{
	if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
		CPoint point;
		GetCursorPos(&point);
		// 坐标转化
		CPoint remote = UserPoint2RemoteScreenPoint(point, true);
		// 封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;  //左键
		event.nAction = 0; //单击
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd() , 5, true, (BYTE*)&event, sizeof(event));
	}
}


void CWatchDialog::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类

	//CDialogEx::OnOK();
}


void CWatchDialog::OnBnClickedBtnLock()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),7);
	// TODO: 在此添加控件通知处理程序代码
}


void CWatchDialog::OnBnClickedBtnUnlock()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),8);
	// TODO: 在此添加控件通知处理程序代码
}


