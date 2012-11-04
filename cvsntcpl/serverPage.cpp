/*	cvsnt control panel
    Copyright (C) 2004-7 Tony Hoyle and March-Hare Software Ltd

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License version 2.1 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
// serverPage.cpp : implementation file
//

#include "stdafx.h"
#include "cvsnt.h"
#include "serverPage.h"
#include "UsageDialog.h"
#include ".\serverpage.h"

/////////////////////////////////////////////////////////////////////////////
// CserverPage property page

IMPLEMENT_DYNCREATE(CserverPage, CTooltipPropertyPage)

typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);

#ifndef PRODUCT_ULTIMATE /* If not using vista/2008 SDK, define them */
// It's truly amazing how many 'editions' of windows there are..
#define PRODUCT_UNDEFINED						0x00000000
#define PRODUCT_ULTIMATE						0x00000001
#define PRODUCT_HOME_BASIC						0x00000002
#define PRODUCT_HOME_PREMIUM					0x00000003
#define PRODUCT_ENTERPRISE						0x00000004
#define PRODUCT_HOME_BASIC_N					0x00000005
#define PRODUCT_BUSINESS						0x00000006
#define PRODUCT_STANDARD_SERVER					0x00000007
#define PRODUCT_DATACENTER_SERVER				0x00000008
#define PRODUCT_SMALLBUSINESS_SERVER			0x00000009
#define PRODUCT_ENTERPRISE_SERVER				0x0000000A
#define PRODUCT_STARTER							0x0000000B
#define PRODUCT_STANDARD_SERVER_CORE			0x0000000D
#define PRODUCT_DATACENTER_SERVER_CORE			0x0000000C
#define PRODUCT_ENTERPRISE_SERVER_CORE			0x0000000E
#define PRODUCT_ENTERPRISE_SERVER_IA64			0x0000000F
#define PRODUCT_BUSINESS_N						0x00000010
#define PRODUCT_WEB_SERVER						0x00000011
#define PRODUCT_CLUSTER_SERVER					0x00000012
#define PRODUCT_HOME_SERVER						0x00000013
#define PRODUCT_STORAGE_EXPRESS_SERVER			0x00000014
#define PRODUCT_STORAGE_STANDARD_SERVER			0x00000015
#define PRODUCT_STORAGE_WORKGROUP_SERVER		0x00000016
#define PRODUCT_STORAGE_ENTERPRISE_SERVER		0x00000017
#define PRODUCT_SERVER_FOR_SMALLBUSINESS		0x00000018
#define PRODUCT_SMALLBUSINESS_SERVER_PREMIUM	0x00000019
#define PRODUCT_HOME_PREMIUM_N					0x0000001A
#define PRODUCT_ENTERPRISE_N					0x0000001B
#define PRODUCT_ULTIMATE_N						0x0000001C
#define PRODUCT_WEB_SERVER_CORE					0x0000001D

// Plus a special flag to GetSystemMetrics just for Win2003 R2
#ifndef SM_SERVERR2
#define SM_SERVERR2 89
#endif

#endif

CserverPage::CserverPage() : CTooltipPropertyPage(CserverPage::IDD)
{
}

CserverPage::~CserverPage()
{
}

void CserverPage::DoDataExchange(CDataExchange* pDX)
{
	CTooltipPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CVSNTVERSION, m_stVersion);
	DDX_Control(pDX, IDC_REGISTRATION, m_stRegistration);
	DDX_Control(pDX, IDC_HOSTOS, m_stHostOs);
	DDX_Control(pDX, IDC_UPTIME, m_stUptime);
	DDX_Control(pDX, IDC_USERS, m_stUserCount);
	DDX_Control(pDX, IDC_SIMULTANEOUSUSERS, m_szMaxUsers);
	DDX_Control(pDX, IDC_TIMEPERUSER, m_stAverageTime);
	DDX_Control(pDX, IDC_CHECK1, m_cbSendStatistics);
	DDX_Control(pDX, IDC_LOGO, m_cbLogo);
	DDX_Control(pDX, IDC_SESSIONCOUNT, m_stSessionCount);
}


BEGIN_MESSAGE_MAP(CserverPage, CTooltipPropertyPage)
	ON_BN_CLICKED(IDC_LOGO, OnBnClickedLogo)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_BN_CLICKED(IDC_CHECK1, OnBnClickedCheck1)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CserverPage message handlers

BOOL CserverPage::OnInitDialog() 
{
	CTooltipPropertyPage::OnInitDialog();

	CBitmap *pBitmap = new CBitmap;
	pBitmap->LoadBitmap(IDB_BITMAP1);
	m_cbLogo.SetBitmap(*pBitmap);
	pBitmap->Detach();
	delete pBitmap;

	m_stVersion.SetWindowText(CVSNT_PRODUCTVERSION_STRING_T);
	TCHAR os[256];
	GetOsVersion(os);
	m_stHostOs.SetWindowText(os);

	union { __time64_t t; BYTE data[8]; } servtime;
	DWORD dwLen=sizeof(servtime.data),dwType;
	cvs::string uptime,tmp;
	if(!RegQueryValueEx(g_hServerKey,_T("StartTime"),NULL,&dwType,servtime.data,&dwLen) && dwType==REG_BINARY)
	{
		__time64_t then = servtime.t,now;
		_time64(&now);
		if(then>now) uptime="??";
		else
		{
			unsigned elapsed = (unsigned)(now-then);
			if(elapsed>43200)
			{
				cvs::sprintf(tmp,32,"%u days, ",elapsed/43200);
				elapsed-=(elapsed/43200)*43200;
				uptime+=tmp;
			}
			if(elapsed>3600 || uptime.size())
			{
				cvs::sprintf(tmp,32,"%u hours, ",elapsed/3600);
				elapsed-=(elapsed/3600)*3600;
				uptime+=tmp;
			}
			cvs::sprintf(tmp,32,"%u minutes",elapsed/60);
			uptime+=tmp;
		}
	}
	else
		uptime="??";

	m_stUptime.SetWindowText(cvs::wide(uptime.c_str()));

	DWORD dwMaxUsers;
	dwLen=sizeof(dwMaxUsers);
	if(!RegQueryValueEx(g_hServerKey,_T("MaxUsers"),NULL,&dwType,(LPBYTE)&dwMaxUsers,&dwLen) && dwType==REG_DWORD)
		cvs::sprintf(tmp,32,"%u",dwMaxUsers);
	else
		tmp="??";
		
	m_szMaxUsers.SetWindowText(cvs::wide(tmp.c_str()));

	DWORD dwAverageTime;
	dwLen=sizeof(dwAverageTime);
	if(!RegQueryValueEx(g_hServerKey,_T("AverageTime"),NULL,&dwType,(LPBYTE)&dwAverageTime,&dwLen) && dwType==REG_DWORD)
		cvs::sprintf(tmp,32,"%u.%02u seconds",dwAverageTime/1000,(dwAverageTime%1000)/10);
	else
		tmp="??";
		
	m_stAverageTime.SetWindowText(cvs::wide(tmp.c_str()));

	DWORD dwSessionCount;
	dwLen=sizeof(dwSessionCount);
	if(!RegQueryValueEx(g_hServerKey,_T("SessionCount"),NULL,&dwType,(LPBYTE)&dwSessionCount,&dwLen) && dwType==REG_DWORD)
		cvs::sprintf(tmp,32,"%u",dwSessionCount);
	else
		tmp="??";
		
	m_stSessionCount.SetWindowText(cvs::wide(tmp.c_str()));

	DWORD dwUserCount;
	dwLen=sizeof(dwUserCount);
	if(!RegQueryValueEx(g_hServerKey,_T("UserCount"),NULL,&dwType,(LPBYTE)&dwUserCount,&dwLen) && dwType==REG_DWORD)
		cvs::sprintf(tmp,32,"%u",dwUserCount);
	else
		tmp="??";
		
	m_stUserCount.SetWindowText(cvs::wide(tmp.c_str()));

	tmp.resize(256);
	dwLen=tmp.size();
	if(!g_hInstallerKey || RegQueryValueExA(g_hInstallerKey,"serversuite",NULL,&dwType,(LPBYTE)tmp.data(),&dwLen) || dwType!=REG_SZ)
	{
		dwLen=tmp.size();
		if(!g_hInstallerKey || RegQueryValueExA(g_hInstallerKey,"server",NULL,&dwType,(LPBYTE)tmp.data(),&dwLen) || dwType!=REG_SZ)
		{
			dwLen=tmp.size();
			if(!g_hInstallerKey || RegQueryValueExA(g_hInstallerKey,"servertrial",NULL,&dwType,(LPBYTE)tmp.data(),&dwLen) || dwType!=REG_SZ)
				tmp="??";
			else
				tmp.resize(dwLen);
		}
		else
			tmp.resize(dwLen);
	}
	else
		tmp.resize(dwLen);

	m_stRegistration.SetWindowText(cvs::wide(tmp.c_str()));

	DWORD dwSendStatistics;
	dwLen=sizeof(dwSendStatistics);
	if(RegQueryValueEx(g_hServerKey,_T("SendStatistics"),NULL,&dwType,(LPBYTE)&dwSendStatistics,&dwLen))
	{
		PostMessage(WM_COMMAND,IDC_BUTTON1);
		dwSendStatistics=1;
	}

	m_cbSendStatistics.SetCheck(dwSendStatistics?1:0);
	if(!g_bPrivileged) m_cbSendStatistics.EnableWindow(FALSE);

	return TRUE; 
}

void CserverPage::OnBnClickedLogo()
{
	ShellExecute(NULL,_T("open"),_T("http://www.march-hare.com/cvspro"),NULL,NULL,SW_SHOWNORMAL);
}

// From MSDN
void CserverPage::GetOsVersion(LPTSTR os)
{
	DWORD dwType;

	OSVERSIONINFOEX vi = {sizeof(OSVERSIONINFOEX)};
	GetVersionEx((OSVERSIONINFO*)&vi);

	SYSTEM_INFO si = {0};
	PGNSI pGNSI = (PGNSI)GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "GetNativeSystemInfo");
	if(pGNSI) pGNSI(&si);
	else GetSystemInfo(&si);

	if (vi.dwPlatformId==VER_PLATFORM_WIN32_NT && vi.dwMajorVersion >4)
	{
		if (vi.dwMajorVersion == 6 && vi.dwMinorVersion == 0)
		{
			if(vi.wProductType == VER_NT_WORKSTATION)
				lstrcpy(os,_T("Windows Vista"));
			else lstrcpy(os,_T("Windows Server 2008"));

			PGPI pGPI = (PGPI) GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "GetProductInfo");
			pGPI( 6, 0, 0, 0, &dwType);

			switch( dwType )
			{
			case PRODUCT_ULTIMATE:
				lstrcat(os, _T(" Ultimate"));
				break;
			case PRODUCT_HOME_PREMIUM:
				lstrcat(os,_T(" Home Premium"));
				break;
			case PRODUCT_HOME_BASIC:
				lstrcat(os,_T(" Home Basic"));
				break;
			case PRODUCT_ENTERPRISE:
				lstrcat(os,_T(" Enterprise"));
				break;
			case PRODUCT_BUSINESS:
				lstrcat(os,_T(" Business"));
				break;
			case PRODUCT_STARTER:
				lstrcat(os, _T(" Starter"));
				break;
			case PRODUCT_CLUSTER_SERVER:
				lstrcat(os, _T(" Cluster Server"));
				break;
			case PRODUCT_DATACENTER_SERVER:
				lstrcat(os, _T(" Datacenter"));
				break;
			case PRODUCT_DATACENTER_SERVER_CORE:
				lstrcat(os, _T(" Datacenter (core)"));
				break;
			case PRODUCT_ENTERPRISE_SERVER:
				lstrcat(os,_T(" Enterprise"));
				break;
			case PRODUCT_ENTERPRISE_SERVER_CORE:
				lstrcat(os,_T(" Enterprise (core)"));
				break;
			case PRODUCT_ENTERPRISE_SERVER_IA64:
				lstrcat(os, _T(" Enterprise for Itanium"));
				break;
			case PRODUCT_SMALLBUSINESS_SERVER:
				lstrcat(os, _T(" Small Business Server"));
				break;
			case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
				lstrcat(os, _T(" Small Business Server Premium"));
				break;
			case PRODUCT_STANDARD_SERVER:
				lstrcat(os, _T(" Standard"));
				break;
			case PRODUCT_STANDARD_SERVER_CORE:
				lstrcat(os, _T(" Standard (core)"));
				break;
			case PRODUCT_WEB_SERVER:
				lstrcat(os, _T(" Web Server"));
				break;
			default:
				{
					TCHAR t[64];
					_sntprintf(t,64,_T("Unknown type %d"),dwType);
					lstrcat(os,t);
				}
				break;
			}
			if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
			lstrcat(os, _T(" (64bit)"));
			else if (si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL )
			lstrcat(os, _T(" (32-bit)"));
		}

		if ( vi.dwMajorVersion == 5 && vi.dwMinorVersion == 2 )
		{
			if( GetSystemMetrics(SM_SERVERR2) )
			lstrcpy(os,_T("Windows 2003 R2"));
			else if ( vi.wSuiteMask==VER_SUITE_STORAGE_SERVER )
			lstrcpy(os,_T("Windows Storage Server 2003"));
			else if( vi.wProductType == VER_NT_WORKSTATION &&
					si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
			lstrcpy(os,_T("Windows XP x64"));
			else
			lstrcpy(os,_T("Windows 2003"));

			// Test for the server type.
			if ( vi.wProductType != VER_NT_WORKSTATION )
			{
				if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64 )
				{
					if( vi.wSuiteMask & VER_SUITE_DATACENTER )
						lstrcat(os, _T(" Datacenter for Itanium"));
					else if( vi.wSuiteMask & VER_SUITE_ENTERPRISE )
						lstrcat(os, _T(" Enterprise for Itanium"));
				}
				else if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
				{
					if( vi.wSuiteMask & VER_SUITE_DATACENTER )
						lstrcat(os, _T(" Datacenter x64"));
					else if( vi.wSuiteMask & VER_SUITE_ENTERPRISE )
						lstrcat(os, _T("Enterprise x64"));
					else
						lstrcat(os, _T("Standard x64") );
				}
				else
				{
					if ( vi.wSuiteMask & VER_SUITE_COMPUTE_SERVER )
						lstrcat(os, _T(" Compute Cluster") );
					else if( vi.wSuiteMask & VER_SUITE_DATACENTER )
						lstrcat(os, _T(" Datacenter"));
					else if( vi.wSuiteMask & VER_SUITE_ENTERPRISE )
						lstrcat(os, _T("Enterprise"));
					else if ( vi.wSuiteMask & VER_SUITE_BLADE )
						lstrcat(os, _T(" Web"));
					else
						lstrcat(os, _T(" Standard"));
				}
			}
		}

		if (vi.dwMajorVersion == 5 && vi.dwMinorVersion == 1)
		{
			lstrcpy(os,_T("Windows XP "));
			if( vi.wSuiteMask & VER_SUITE_PERSONAL )
			lstrcat(os,_T("Home"));
			else
			lstrcat(os,_T("Professional"));
		}

		if ( vi.dwMajorVersion == 5 && vi.dwMinorVersion == 0 )
		{
			lstrcpy(os,_T("Windows 2000"));

			if ( vi.wProductType == VER_NT_WORKSTATION )
				lstrcat(os,_T(" Professional"));
			else 
			{
				if( vi.wSuiteMask & VER_SUITE_DATACENTER )
					lstrcat(os,_T(" Datacenter Server"));
				else if( vi.wSuiteMask & VER_SUITE_ENTERPRISE )
					lstrcat(os,_T(" Advanced Server"));
				else
					lstrcat(os,_T(" Server"));
			}
		}
	}
	else
	{
		// NT4, Win9x
		lstrcpy(os,_T("NT4 or older"));
	}

	if(*vi.szCSDVersion)
	{
		lstrcat(os,_T(" "));
		lstrcat(os,vi.szCSDVersion);
	}

	/* Vmware/virtualpc detection isn't actually that useful, but if we do it now we can record
	it later if it becomes sensible to do so */
	if(IsInsideVMWare())
		lstrcat(os,_T(" (vmware)"));
	else if(IsInsideVPC())
		lstrcat(os,_T(" (virtualpc)"));
}

bool CserverPage::IsInsideVMWare()
{
  bool rc = true;

  __try
  {
    __asm
    {
      push   edx
      push   ecx
      push   ebx

      mov    eax, 'VMXh'
      mov    ebx, 0 // any value but not the MAGIC VALUE
      mov    ecx, 10 // get VMWare version
      mov    edx, 'VX' // port number

      in     eax, dx // read port
                     // on return EAX returns the VERSION
      cmp    ebx, 'VMXh' // is it a reply from VMWare?
      setz   [rc] // set return value

      pop    ebx
      pop    ecx
      pop    edx
    }
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
    rc = false;
  }

  return rc;
}

// IsInsideVPC's exception filter
DWORD __forceinline CserverPage::IsInsideVPC_exceptionFilter(LPEXCEPTION_POINTERS ep)
{
  PCONTEXT ctx = ep->ContextRecord;

  ctx->Ebx = -1; // Not running VPC
  ctx->Eip += 4; // skip past the "call VPC" opcodes
  return EXCEPTION_CONTINUE_EXECUTION;
  // we can safely resume execution since we skipped faulty instruction
}

// High level language friendly version of IsInsideVPC()
bool CserverPage::IsInsideVPC()
{
  bool rc = false;

  __try
  {
    _asm push ebx
    _asm mov  ebx, 0 // It will stay ZERO if VPC is running
    _asm mov  eax, 1 // VPC function number

    // call VPC 
    _asm __emit 0Fh
    _asm __emit 3Fh
    _asm __emit 07h
    _asm __emit 0Bh

    _asm test ebx, ebx
    _asm setz [rc]
    _asm pop ebx
  }
  // The except block shouldn't get triggered if VPC is running!!
  __except(IsInsideVPC_exceptionFilter(GetExceptionInformation()))
  {
  }

  return rc;
}

void CserverPage::OnBnClickedButton1()
{
	CUsageDialog dlg;
	int nRet = dlg.DoModal();
	if(g_bPrivileged)
	{
		if(nRet==IDOK)
			m_cbSendStatistics.SetCheck(1);
		else
			m_cbSendStatistics.SetCheck(0);
		SetModified();
	}
}

void CserverPage::OnBnClickedCheck1()
{
	SetModified();
}

BOOL CserverPage::OnApply()
{
	if(g_bPrivileged)
	{
		DWORD dwSendStatistics = m_cbSendStatistics.GetCheck();
		RegSetValueEx(g_hServerKey,_T("SendStatistics"),NULL,REG_DWORD,(LPBYTE)&dwSendStatistics,sizeof(dwSendStatistics));
	}

	return CTooltipPropertyPage::OnApply();
}
