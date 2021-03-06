
// MRRKinectDlg.cpp : implementation file
//
#include "stdafx.h"
#include <afxwin.h>
#include "MRRKinect.h"
#include "MRRKinectDlg.h"
#include <afxdialogex.h>

//for test
#define _WINSOCKAPI_
#include <windows.h>
#include <time.h>
#include <stdio.h>
#include <iostream>
#include <ctime>

using namespace std;
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#define	_JOINT_RHAND 15
#define maxFrameNum 10000

bool g_RunKinect = true;
HANDLE g_hKinectThread;
int gnSystemStatus = _SS_CALIB;
int gCalibStatus = 1;
POINT3D gpMarkerSet[3];
int gMarkerCount = 0;
int gDataStreamStatus = _DS_CLOSE;
bool gIsSkeletonUpdated = false;
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

struct timezone
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};

BOOL isMarkerInit()
{
	if(gMarkerCount >= 3 )
		return true;
	else
		return false;

}
// Definition of a gettimeofday function

void getTimeofDay(struct timeval *tv, struct timezone *tz)
{
// Define a structure to receive the current Windows filetime
  FILETIME ft;
 
// Initialize the present time to 0 and the timezone to UTC
  unsigned __int64 tmpres = 0;
  static int tzflag = 0;
 
  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);
 
// The GetSystemTimeAsFileTime returns the number of 100 nanosecond 
// intervals since Jan 1, 1601 in a structure. Copy the high bits to 
// the 64 bit tmpres, shift it left by 32 then or in the low 32 bits.
    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;
 
// Convert to microseconds by dividing by 10
    tmpres /= 10;
 
// The Unix epoch starts on Jan 1 1970.  Need to subtract the difference 
// in seconds from Jan 1 1601.
    tmpres -= DELTA_EPOCH_IN_MICROSECS;
 
// Finally change microseconds to seconds and place in the seconds value. 
// The modulus picks up the microseconds.
    tv->tv_sec = (long)(tmpres / 1000000UL);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }
 
  if (NULL != tz)
  {
    if (!tzflag)
    {
      _tzset();
      tzflag++;
    }
  
// Adjust for the timezone west of Greenwich
      tz->tz_minuteswest = _timezone / 60;
    tz->tz_dsttime = _daylight;
  }
 
  return;
}
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMRRKinectDlg dialog

CMRRKinectDlg::CMRRKinectDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMRRKinectDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pImgProc = new ImageProc;
	m_pModel = new ColorModel;
	m_pCalib = new KinectCalibration;

	m_nDisplayType = 3;
}

void CMRRKinectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMRRKinectDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDCANCEL, &CMRRKinectDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_CALIB, &CMRRKinectDlg::OnBnClickedButtonCalib)
	ON_WM_HSCROLL()
	ON_WM_MOUSEMOVE()
	ON_BN_CLICKED(IDC_BUTTON_TRACK, &CMRRKinectDlg::OnBnClickedButtonTrack)
	ON_BN_CLICKED(IDC_BUTTON_RESET, &CMRRKinectDlg::OnBnClickedButtonReset)
	ON_BN_CLICKED(IDC_BUTTON_SETMODEL, &CMRRKinectDlg::OnBnClickedButtonSetmodel)
	ON_BN_CLICKED(IDC_BUTTON_ARCHIVE, &CMRRKinectDlg::OnBnClickedButtonArchive)
	ON_BN_CLICKED(IDC_BUTTON_ENDARCHIVE, &CMRRKinectDlg::OnBnClickedButtonEndarchive)
	ON_BN_CLICKED(IDC_BUTTON_SAVECALIB, &CMRRKinectDlg::OnBnClickedButtonSavecalib)
	ON_BN_CLICKED(IDC_BUTTON_TESTCALIB, &CMRRKinectDlg::OnBnClickedButtonTestcalib)
	ON_BN_CLICKED(IDC_BUTTON_RESETCALIB, &CMRRKinectDlg::OnBnClickedButtonResetcalib)
	ON_BN_CLICKED(IDC_BUTTON_RECORD, &CMRRKinectDlg::OnBnClickedButtonRecord)
	ON_BN_CLICKED(IDC_BUTTON_RECORDEND, &CMRRKinectDlg::OnBnClickedButtonRecordEnd)
	ON_COMMAND(IDC_RADIO1, &CMRRKinectDlg::OnRadio1)
	ON_COMMAND(IDC_RADIO2, &CMRRKinectDlg::OnRadio2)
	ON_COMMAND(IDC_RADIO3, &CMRRKinectDlg::OnRadio3)
	ON_COMMAND(IDC_RADIO4, &CMRRKinectDlg::OnRadio4)
	ON_BN_CLICKED(IDC_BUTTON_ENDRUN, &CMRRKinectDlg::OnBnClickedButtonEndRun)
END_MESSAGE_MAP()


// CMRRKinectDlg message handlers

BOOL CMRRKinectDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	InitImgWnd();
	InitKinect();


	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMRRKinectDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}
// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMRRKinectDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}
// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMRRKinectDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMRRKinectDlg::InitImgWnd() {
	CRect rect;	
	int nBorder = 8;

	GetDlgItem( IDC_FULLFRAME )->GetWindowRect( rect );
	ScreenToClient( rect );
	rect.left += nBorder;
	rect.right -= nBorder-1;
	rect.top += nBorder+10;
	rect.bottom -= nBorder+10;
	m_wndFull = new CDrawWnd();
	m_wndFull->Create( WS_VISIBLE , rect, this, IDR_FULLFRAME );

	CheckRadioButton(IDC_RADIO1, IDC_RADIO4, IDC_RADIO3);

}


DWORD WINAPI ShowStreams(LPVOID lpParam) {
	CMRRKinectDlg* dlg = (CMRRKinectDlg*)(lpParam);
	KinectSensor* kinect = dlg->getKinect();


	int w = kinect->getRGBImg()->width();
	int h = kinect->getRGBImg()->height();
	int wb = kinect->getRGBImg()->widthBytes();

	CImgWnd* fullWnd = dlg->getFullWnd();

	switch( dlg->DisplayType() ){
			case 1: wb = kinect->getDepthImg()->widthBytes();
					fullWnd->ShowImg16( w, h, wb, kinect->getDepthImg()->getData());//Display depthbreak;
				break;
			case 2: wb = kinect->getRGBImg()->widthBytes();
					//fullWnd->ShowImg(w, h, wb, kinect->getRGBImg()->getData());//Display imagebreak;
					fullWnd->ShowImg(w, h, wb, dlg->getTracker()->getImg()->getData());
					fullWnd->Invalidate();
					break;
			case 3: wb = kinect->getRGBImg()->widthBytes();
					fullWnd->ShowImg(w,h,wb,kinect->getRGBImg()->getData());
					if(gIsSkeletonUpdated)
					{
						fullWnd->showSkeleton(dlg->getSkeleton()->getJointPos());
					}
					Rect modRect;
					modRect.x = MODEL_StartX - (int)(MODEL_W/2);
					modRect.y = MODEL_StartY - (int)(MODEL_H/2);
					modRect.width = MODEL_W;
					modRect.height = MODEL_H;
					fullWnd->showRect( modRect );
					//
					///*Rect curRect;
					//curRect.width = dlg->getTracker()->getPrevRect().width;
					//curRect.height = dlg->getTracker()->getPrevRect().height;
					//curRect.x = dlg->getTracker()->getPrevRect().x;
					//curRect.y = dlg->getTracker()->getPrevRect().y;*/
					fullWnd->showHandJoint( dlg->getTracker()->getHandPos() );
					fullWnd->Invalidate();
				break;
			case 0: fullWnd->Invalidate();break;
	}

	//Test code: Edited by Tingfang 10/28
	//fullWnd->ShowImg(w,h,wb,kinect->getRGBImg()->getData());
	//fullWnd->ShowImg(w,h,wb,dlg->getBG()->getRgbBG()->getData());

	//fullWnd->ShowImg(w, h, wb, dlg->getHandTrackData()->getImg()->getData());
	
	//DO NOT USE ENDPOINT

	//WARNING END*/
	if( dlg->getRecorder()->isRun() )
			dlg->getRecorder()->record( kinect->getRGBImg() );

	return 0;
}

DWORD WINAPI ReceiveDashProcess(LPVOID lpParam){
	CMRRKinectDlg* dlg = (CMRRKinectDlg*)(lpParam);
	KSFrameDataReceiver receiver;
	OSFrameData* frameData = dlg->getOSFrameData();
	int nControlStatus = 0;
	receiver.openClient();

	while(1)
	{
		receiver.receiveData();
		frameData->setState( receiver.getData()->getState() );
		cout << "The receiver Data Status is:" << frameData->getState()<< endl;
		nControlStatus = frameData->getState();
		switch( nControlStatus ){
		case -1: break;
		case 1:dlg->OnBnClickedButtonSetmodel();break;
		case 2:dlg->OnBnClickedButtonRecord();break;
		case 3:dlg->OnBnClickedButtonRecordEnd();break;
		case 0:dlg->OnBnClickedCancel();break;
		default: break;
		}
	}
	receiver.closeClient();
	return 0;
}

DWORD WINAPI KinectThread(LPVOID lpParam) {
	CMRRKinectDlg* dlg = (CMRRKinectDlg*)(lpParam);
	KinectOpenNI* kinect = dlg->getKinect();
	KinectCalibration* calib = dlg->getCalibration();
	Background* bg = dlg->getBG();
	KinectSkeletonOpenNI* skeleton = dlg->getSkeleton();
	KSHandTrackData* handData = dlg->getTracker();
	KSTorsoData* torsoData = dlg->getTorsoData();
	KSFrameData* frameData = dlg->getFrameData();
	OSFrameData* osFrameData = dlg->getOSFrameData();
	KSElbowData* elbowData = dlg->getElbowData();
	KSArchivingData* archiveData = dlg->getArchivingData();
	
	struct timeval timeStamp;
	double lastFrameTime = 0.0f;
	double currentFrameTime = 0.0f;
	int frameID = 0;
	KSFrameDataSender* sender = dlg->getSender();

	sender->openServer();
	//receiver->openClient();

	HANDLE hReceiveDashProcess = CreateThread(NULL, 0, ReceiveDashProcess, dlg, 0, NULL);	
	while(g_RunKinect) {

		lastFrameTime = currentFrameTime;
		frameID = frameID + 1;

		kinect->update();
		gIsSkeletonUpdated = skeleton->update();
		if( gIsSkeletonUpdated )
		{
			gnSystemStatus = _SS_TRACK;
			if(gDataStreamStatus == _DS_CLOSE)
				gDataStreamStatus = _DS_WAITFORPOSE ;
		}
		else
		{
			gnSystemStatus = _SS_CALIB;
			if(gDataStreamStatus > _DS_CLOSE )
				gDataStreamStatus = _DS_CLOSE;
		}


		//cout << "System status: " << gnSystemStatus << endl;

		switch( gnSystemStatus ){
			case _SS_REST: break;
			case _SS_CALIB:
				{
					switch( gCalibStatus ){
						case 0:
							if( isMarkerInit() )
							{	
								calib->startCalib( gpMarkerSet, kinect->getDepthGenerator());
								break;
							}

							cout << "Please Identify the 3 marker positions" << endl;break;
							
						case 1:
							calib->startCalib( kinect->getRGBImg(), kinect->getDepthImg(),kinect->getDepthGenerator() );break;	
						default:
							AfxMessageBox(_T("Please choose Calibration Status"));break;
					}
				}
			case _SS_TRACK:	
				{

					if( gIsSkeletonUpdated )
					{

						//------------
						//UPDATE DATA
						//------------
						//1. handData 2. torsoData 3. elbowData 
						 // handData->update(            
							//kinect->getRGBImg(),
							//kinect->getDepthImg(),
							//handData->getHandPos().x,
							//handData->getHandPos().y,
							//handData->getPrevRect().width,                                    //4.12 Can we change the ROI size depending on the tracking performance? if tracking lost, 
							//handData->getPrevRect().height 
							//);
 

						bool isTorsoUpdated =torsoData->update( kinect->getDepthImg(), skeleton, calib, kinect->getDepthGenerator() );
						bool isElbowUpdated =elbowData->update( skeleton, kinect->getDepthGenerator());

						//cout << isTorsoUpdated << ",  " << isElbowUpdated << endl;
						if( isTorsoUpdated && isElbowUpdated && gDataStreamStatus == _DS_WAITFORPOSE)
						{
							gDataStreamStatus = _DS_READY;
						}


						POINT3D handPos3D = calib->cvtIPtoGP(handData->getHandPos(),kinect->getDepthGenerator());
						//cout << "hand Position:" << handPos3D.x << ", " << handPos3D.y << ", " << handPos3D.z << endl;
						//cout << "dataStreamStatus: " << gDataStreamStatus << endl;
						

						getTimeofDay(&timeStamp, NULL);
						currentFrameTime = timeStamp.tv_sec + (timeStamp.tv_usec/1000000.0);						
						
						frameData->update(frameID, 
							currentFrameTime,
							skeleton->getJoint3DPosAt(XN_SKEL_LEFT_SHOULDER).x,
							skeleton->getJoint3DPosAt(XN_SKEL_LEFT_SHOULDER).y,
							skeleton->getJoint3DPosAt(XN_SKEL_LEFT_SHOULDER).z,
							skeleton->getJoint3DPosAt(XN_SKEL_RIGHT_SHOULDER).x,
							skeleton->getJoint3DPosAt(XN_SKEL_RIGHT_SHOULDER).y,
							skeleton->getJoint3DPosAt(XN_SKEL_RIGHT_SHOULDER).z,
							skeleton->getJoint3DPosAt(XN_SKEL_TORSO).x,
							skeleton->getJoint3DPosAt(XN_SKEL_TORSO).y,
							skeleton->getJoint3DPosAt(XN_SKEL_TORSO).z,
							torsoData->getTorsoComps(),
							torsoData->getShoulderRot(),
							elbowData->getElbowOpening(),
							handPos3D.x,
							handPos3D.y,
							handPos3D.z,
							gDataStreamStatus
							);																				//if tracking is success, we record the data, frame data updates.

						
						sender->getData()->update(
							frameData->getFrameID(),
							frameData->getTimestamp(),
							frameData->getLShoulderX(),
							frameData->getLShoulderY(),
							frameData->getLShoulderZ(),
							frameData->getRShoulderX(),
							frameData->getRShoulderY(),
							frameData->getRShoulderZ(),
							frameData->getTorsoX(),
							frameData->getTorsoY(),
							frameData->getTorsoZ(),
							frameData->getTorsoComp(),
							frameData->getShoulderRot(),
							frameData->getElbowOpen(),
							frameData->getHandX(),
							frameData->getHandY(),
							frameData->getHandZ(),
							frameData->getStatus()
							);
						

						sender->sendData();
						//receiver->receiveData();

						//cout << "Sended Status! " << sender->getData()->getStatus() << endl;
						if( archiveData->isArchiving())
							archiveData->addAFrame(dlg->getFrameData());
						
						//------------
						//DISPLAY DATA
						//------------
						CString str;
						str.Format(_T("%lf"), dlg->getTorsoData()->getTorsoComps());
						dlg->SetDlgItemText(IDC_EDIT12, str);
						str.Format(_T("%lf"), dlg->getElbowData()->getElbowOpening());
						dlg->SetDlgItemText(IDC_EDIT13, str);
						str.Format(_T("%lf"), dlg->getTorsoData()->getShoulderRot());
						dlg->SetDlgItemText(IDC_EDIT14, str);
					}

					break;
				}
		}

		HANDLE hShowStreams = CreateThread(NULL, 0, ShowStreams, dlg, 0, NULL);
		WaitForSingleObject(hShowStreams, INFINITE);
		getTimeofDay(&timeStamp, NULL);
		currentFrameTime = timeStamp.tv_sec + (timeStamp.tv_usec/1000000.0);
		//cout << "THE STATE IS BITCH: " << dlg->getReceiver()->getData()->getState() << endl;
		//Display Data info
		CString str;
		str.Format(_T("%d"), frameID);
		dlg->SetDlgItemText(IDC_EDIT4, str);
		str.Format(_T("%lf"), 1.0f/(currentFrameTime-lastFrameTime));
		dlg->SetDlgItemText(IDC_EDIT5, str);

	}
	sender->closeServer();
	//receiver->closeClient();
	return 0;
}

void CMRRKinectDlg::InitKinect() {
	
	kinect.init();

	int nWidth = kinect.getRGBImg()->width();
	int	nHeight = kinect.getRGBImg()->height(); 
	m_pBG = new Background(nWidth,nHeight);	//Is it good to put it here?
	m_pSkeleton = new KinectSkeletonOpenNI(kinect.getContext(), kinect.getDepthGenerator(), 24);
	
	m_pFrameData = new KSFrameData;
	m_pOSFrameData = new OSFrameData;
	m_pTracker = new KSHandTrackMeanshiftTracker(nWidth,nHeight);
	m_pTorsoData = new KSTorsoData;
	m_pElbowData = new KSElbowData;
	m_pArchivingData = new KSArchivingData;
	m_pVideoRecorder = new KSUtilsVideoRecorder;
	m_pSender = new KSFrameDataSender;
	//m_pReceiver = new KSFrameDataReceiver;
	//m_pFilter = new KSUtilsMAFilter(2);

	for( int i = 0 ; i < 3 ; ++i )
	{
		gpMarkerSet[i].x = 0.0f;
		gpMarkerSet[i].y = 0.0f;
		gpMarkerSet[i].z = 0.0f;
	}
	gMarkerCount = 0;

	kinect.open();

	g_hKinectThread = CreateThread(NULL, 0, KinectThread, this, 0, NULL);

}

void CMRRKinectDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	g_RunKinect = false;
	WaitForSingleObject(g_hKinectThread, 200);
	CDialogEx::OnCancel();
}

void CMRRKinectDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default
	CString str;
	UpdateData(TRUE);

	//if( pScrollBar == this->GetDlgItem(IDC_SLIDER1))
	//{	
	//	m_slider1 = (CSliderCtrl*)pScrollBar;
	//	m_slider1->SetRange(0,400);
	//	m_pCalib->setValue1(m_slider1->GetPos());
	//	str.Format(_T("%d"), m_pCalib->getValue1());
	//	SetDlgItemText(IDC_EDIT1, str);
	//}
	UpdateData(FALSE);
	//CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CMRRKinectDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

}

BOOL CMRRKinectDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if(pMsg->message == WM_LBUTTONDOWN && gnSystemStatus == 1 )
	{
		CPoint point;
		GetCursorPos(&point);
		ScreenToClient( &point );

		CRect rect;
		float h,s,v; 
		GetDlgItem( IDC_FULLFRAME )->GetWindowRect( &rect );
		ScreenToClient( &rect );
		point.x = point.x - rect.TopLeft().x - 4;
		point.y = point.y - rect.TopLeft().y - 11;

		gpMarkerSet[gMarkerCount].x = point.x;
		gpMarkerSet[gMarkerCount].y = point.y;
		gpMarkerSet[gMarkerCount].z = 0;

		gMarkerCount += 1;
	}
	if(pMsg->message == WM_KEYDOWN)
	{
		//listen to the specific keyboard message
		if(pMsg->wParam == 'T')
			m_pCalib->setCalibStage(1); //test calib
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

void CMRRKinectDlg::OnBnClickedButtonCalib()
{
	// TODO: Add your control notification handler code here
	gnSystemStatus = _SS_CALIB;
}

void CMRRKinectDlg::OnBnClickedButtonTrack()
{
	// TODO: Add your control notification handler code here
	gnSystemStatus = _SS_TRACK;
}

void CMRRKinectDlg::OnBnClickedButtonReset()
{
	// TODO: Add your control notification handler code here
	gnSystemStatus = _SS_REST;
}

void CMRRKinectDlg::OnBnClickedButtonSetmodel()
{
	cout << "resonsed" << endl;
	// TODO: Add your control notification handler code here
	if( gDataStreamStatus == _DS_READY )
	{
	POINT3D shoulderCenterPos;
	shoulderCenterPos.x = (m_pSkeleton->getJointPosAt(XN_SKEL_LEFT_SHOULDER).x + m_pSkeleton->getJointPosAt(XN_SKEL_RIGHT_SHOULDER).x )/2;
	shoulderCenterPos.y = (m_pSkeleton->getJointPosAt(XN_SKEL_LEFT_SHOULDER).y + m_pSkeleton->getJointPosAt(XN_SKEL_RIGHT_SHOULDER).y )/2;
	shoulderCenterPos.z = 0.0f;

	m_pTorsoData->setCurrentPos( shoulderCenterPos );
	m_pTorsoData->setRestLShoulderPos( m_pSkeleton->getJoint3DPosAt(XN_SKEL_LEFT_SHOULDER) );
	m_pTorsoData->setRestRShoulderPos( m_pSkeleton->getJoint3DPosAt(XN_SKEL_RIGHT_SHOULDER) );
	m_pTorsoData->setRestTorsoPos( m_pSkeleton->getJoint3DPosAt(XN_SKEL_TORSO) );

	Plane3D plane;
	plane = m_pTorsoData->calcPlaneFrom3Points(m_pTorsoData->getRestLShoulderPos(), m_pTorsoData->getRestRShoulderPos(), m_pTorsoData->getRestTorsoPos());
	m_pTorsoData->setRestPlane( plane );
	
	Rect modRect;
	modRect.x = MODEL_StartX - (int)(MODEL_W/2);
	modRect.y = MODEL_StartY - (int)(MODEL_H/2);
	modRect.width = MODEL_W;
	modRect.height = MODEL_H;

	m_pTracker->init( kinect.getRGBImg(), modRect );

gDataStreamStatus = _DS_TRACK;
	/*DO NOT USE ENDPOINT TRACK
	m_pHandTrackData->setInit( false );
	
	m_pHandTrackData->setRestPosDepth(torso3DPos.z);
	gnPointMode = 2;
	*/
	m_pTracker->setRestPosDepth( m_pTorsoData->getRestTorsoPos().z );
	if(m_pTracker!=NULL)
		m_pTracker->setReady(true);
	if(m_pTorsoData!=NULL)
		m_pTorsoData->setReady(true);
	if(m_pElbowData!=NULL)
		m_pElbowData->setReady(true);
	}
	else
		return;

}

void CMRRKinectDlg::OnBnClickedButtonArchive()
{
	// TODO: Add your control notification handler code here
	//if no data, return;
	if(gDataStreamStatus == _DS_TRACK )
	{
		if( m_pArchivingData != NULL )
			m_pArchivingData->setIsArchiving(true);
	}
	else
		cout << "Not ready to send data!" << endl;
}

void CMRRKinectDlg::OnBnClickedButtonEndarchive()
{
	// TODO: Add your control notification handler code here
	if( m_pArchivingData != NULL )
	{
		m_pArchivingData->saveArchivingData();
		m_pArchivingData->setIsArchiving(false);
	}	
	else
		cout << "Not ready to send data!" << endl;
}

void CMRRKinectDlg::OnBnClickedButtonSavecalib()
{
	// TODO: Add your control notification handler code here
	//if(gnSystemStatus!=_SS_CALIB) return;		
	m_pCalib->saveCalibrationDatatoFile();
	cout << "Coordinate set, please test" << endl;
}

void CMRRKinectDlg::OnBnClickedButtonTestcalib()
{
	// TODO: Add your control notification handler code here
	gCalibStatus = 1;
}

void CMRRKinectDlg::OnBnClickedButtonResetcalib()
{
	// TODO: Add your control notification handler code here
	gCalibStatus = 0;
	for( int i = 0; i< 3; i++ )
	{
		gpMarkerSet[i].x = 0.0f;
		gpMarkerSet[i].y = 0.0f;
		gpMarkerSet[i].z = 0.0f;
	}
	gMarkerCount = 0;
}


void CMRRKinectDlg::OnBnClickedButtonRecord()
{
	// TODO: Add your control notification handler code here
	if(gDataStreamStatus == _DS_TRACK )
	{
		m_pVideoRecorder->setIsRun( true );
		m_pVideoRecorder->init();
	}
	else
		cout << "Not ready to send data!" << endl;
}


void CMRRKinectDlg::OnBnClickedButtonRecordEnd()
{
	// TODO: Add your control notification handler code here
	if(gDataStreamStatus == _DS_TRACK )
	{
		m_pVideoRecorder->close();
	}
	else
		cout << "Not ready to send data!" << endl;

	m_pVideoRecorder->setIsRun( false );
}

void CMRRKinectDlg::OnBnClickedButtonNewcalib()
{
	// TODO: Add your control notification handler code here
	gMarkerCount = 0;
	for( int i = 0 ; i< 3 ; ++i )
	{
		gpMarkerSet[i].x = 0.0f;
		gpMarkerSet[i].y = 0.0f;
		gpMarkerSet[i].z = 0.0f;
	}

}




void CMRRKinectDlg::OnBnClickedButtonLoadcalib()
{
	// TODO: Add your control notification handler code here
	gCalibStatus = 1;
}


void CMRRKinectDlg::OnRadio1()
{
	// TODO: Add your command handler code here
	m_nDisplayType = 1;
}


void CMRRKinectDlg::OnRadio2()
{
	// TODO: Add your command handler code here
	m_nDisplayType = 2;
}


void CMRRKinectDlg::OnRadio3()
{
	// TODO: Add your command handler code here
	m_nDisplayType = 3;
}


void CMRRKinectDlg::OnRadio4()
{
	// TODO: Add your command handler code here
	m_nDisplayType = 0;
}


void CMRRKinectDlg::OnBnClickedButtonEndRun()
{
	// TODO: Add your control notification handler code here
	if(gDataStreamStatus == 2)
		gDataStreamStatus =1;
}
