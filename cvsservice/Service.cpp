/*	cvsnt dispatcher
    Copyright (C) 2004-5 Tony Hoyle and March-Hare Software Ltd

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
// Service.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ServiceMsg.h"
#include <config.h>
#include <cvsapi.h>
#include <cvstools.h>
#include <malloc.h>
#include <io.h>
#include <vector>
#include <shlwapi.h>
#include <io.h>

#define SECURITY_WIN32
#include <security.h>
#include <ntsecapi.h>
#include <ntdsapi.h>
#include <dsgetdc.h>
#include <lm.h>
#include <activeds.h>
#include <shellapi.h>


#include "../version_no.h"
#include "../version_fu.h"

#include <comdef.h>
struct  _LARGE_INTEGER_X {
 struct {
  DWORD LowPart;
  LONG HighPart;
 } u;
};
// This stops the compiler from complaining about negative values in unsigned longs.
#pragma warning(disable:4146)
#pragma warning(disable:4192)
#import <activeds.tlb>  rename("_LARGE_INTEGER","_LARGE_INTEGER_X") rename("GetObject","_GetObject")
#pragma warning(default:4146)
#pragma warning(default:4192)

#define SERVICE_NAMEA "CVSNT"
#define SERVICE_NAME _T("CVSNT")
#define DISPLAY_NAMEA "CVSNT"
#define DISPLAY_NAME _T("CVSNT")
#define NTSERVICE_VERSION_STRING _T("CVSNT Service ") CVSNT_PRODUCTVERSION_STRING_T

static void CALLBACK ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv);
static void CALLBACK ServiceHandler(DWORD fdwControl);
static BOOL NotifySCM(DWORD dwState, DWORD dwWin32ExitCode, DWORD dwProgress);
static char* basename(const char* str);
static LPCSTR GetErrorString();
static void AddEventSource(LPCTSTR szService, LPCTSTR szModule);
static void ReportError(BOOL bError, LPCSTR szError, ...);
static DWORD CALLBACK DoCvsThread(LPVOID lpParam);
static DWORD CALLBACK DoUnisonThread(LPVOID lpParam);
static bool SendStatistics();
static void GetOsVersion(LPTSTR os, LPTSTR servicepack, DWORD& dwMajor, DWORD& dwMinor);

static DWORD   g_dwCurrentState;
static SERVICE_STATUS_HANDLE  g_hService;
static std::vector<SOCKET> g_Sockets;
static BOOL g_bStop = FALSE;
static BOOL g_bTestMode = FALSE;
static LONG g_dwUsers,g_dwMaxUsers,g_dwAverageTime,g_dwSessionCount;

union servtime_t { __time64_t t; BYTE data[8]; };

/* These are from vista SDK */
#define SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO 6
typedef struct _SERVICE_REQUIRED_PRIVILEGES_INFO {  
	LPTSTR pmszRequiredPrivileges;
} SERVICE_REQUIRED_PRIVILEGES_INFO;
/*  */

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

CRITICAL_SECTION g_crit;

void InitServer()
{
	InitializeCriticalSection(&g_crit);
}

void CloseServer()
{
	DeleteCriticalSection(&g_crit);
}

struct ClientLock
{
	ClientLock() { EnterCriticalSection(&g_crit); }
	~ClientLock() { LeaveCriticalSection(&g_crit); }
};

int main(int argc, char* argv[])
{
    SC_HANDLE  hSCManager = NULL, hService = NULL;
    TCHAR szImagePath[MAX_PATH];
	HKEY hk;
	DWORD dwType;
	SERVICE_TABLE_ENTRY ServiceTable[] =
	{
		{ SERVICE_NAME, ServiceMain },
		{ NULL, NULL }
	};
	LPSTR szRoot;

	if(argc==1)
	{
		// Attempt to start service.  If this fails we're probably
		// not running as a service
		if(!StartServiceCtrlDispatcher(ServiceTable)) return 0;
	}
	if(argc<2 || (strcmp(argv[1],"-i") && strcmp(argv[1],"-reglsa") && strcmp(argv[1],"-u") && strcmp(argv[1],"-unreglsa") && strcmp(argv[1],"-test") && strcmp(argv[1],"-v") && strcmp(argv[1],"-sendstats")))
	{
		fprintf(stderr, "CVSNT Service Handler\n\n"
                        "Arguments:\n"
                        "\t%s -i [cvsroot]\tInstall\n"
                        "\t%s -reglsa\tRegister LSA helper\n"
                        "\t%s -u\tUninstall\n"
                        "\t%s -unreglsa\tUnregister LSA helper\n"
                        "\t%s -test\tInteractive run\n"
                        "\t%s -v\tReport version number\n"
						"\t%s -sendstats\tSend statistics\n",
                        basename(argv[0]),basename(argv[0]),
                        basename(argv[0]), basename(argv[0]), 
                        basename(argv[0]), basename(argv[0]),
						basename(argv[0])
                        );
		return -1;
	}

	if(!strcmp(argv[1],"-reglsa"))
	{
		TCHAR lsaBuf[10240];
		DWORD dwLsaBuf;

		if(!CGlobalSettings::isAdmin())
		{
			fprintf(stderr,"Must be run as an administrator to use this option\n");
			return -1;
		}

		if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("SYSTEM\\CurrentControlSet\\Control\\Lsa"),0,KEY_ALL_ACCESS,&hk))
		{
			fprintf(stderr,"Couldn't open LSA registry key, error %d\n",GetLastError());
			return -1;
		}
		dwLsaBuf=sizeof(lsaBuf);
		if(RegQueryValueEx(hk,_T("Authentication Packages"),NULL,&dwType,(BYTE*)lsaBuf,&dwLsaBuf))
		{
			fprintf(stderr,"Couldn't read LSA registry key, error %d\n",GetLastError());
			return -1;
		}
		if(dwType!=REG_MULTI_SZ)
		{
			fprintf(stderr,"LSA key isn't REG_MULTI_SZ!!!\n");
			return -1;
		}
		lsaBuf[dwLsaBuf]='\0';
		TCHAR *p = lsaBuf;
		while(*p)
		{
			if(!_tcscmp(p,_T("setuid")))
				break;
			p+=_tcslen(p)+1;
		}
		if(!*p)
		{
			_tcscpy(p,_T("setuid"));
			dwLsaBuf+=_tcslen(p)+1;
			lsaBuf[dwLsaBuf]='\0';
			if(RegSetValueEx(hk,_T("Authentication Packages"),NULL,dwType,(BYTE*)lsaBuf,dwLsaBuf))
			{
				fprintf(stderr,"Couldn't write LSA registry key, error %d\n",GetLastError());
				return -1;
			}
		}
		return 0;
	}

	if(!strcmp(argv[1],"-unreglsa"))
	{
		TCHAR lsaBuf[10240];
		DWORD dwLsaBuf;

		if(!CGlobalSettings::isAdmin())
		{
			fprintf(stderr,"Must be run as an administrator to use this option\n");
			return -1;
		}

		if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("SYSTEM\\CurrentControlSet\\Control\\Lsa"),0,KEY_ALL_ACCESS,&hk))
		{
			fprintf(stderr,"Couldn't open LSA registry key, error %d\n",GetLastError());
			return -1;
		}
		dwLsaBuf=sizeof(lsaBuf);
		if(RegQueryValueEx(hk,_T("Authentication Packages"),NULL,&dwType,(BYTE*)lsaBuf,&dwLsaBuf))
		{
			fprintf(stderr,"Couldn't read LSA registry key, error %d\n",GetLastError());
			return -1;
		}
		if(dwType!=REG_MULTI_SZ)
		{
			fprintf(stderr,"LSA key isn't REG_MULTI_SZ!!!\n");
			return -1;
		}
		lsaBuf[dwLsaBuf]='\0';
		TCHAR *p = lsaBuf;
		while(*p)
		{
			if(!_tcscmp(p,_T("setuid")))
				break;
			p+=_tcslen(p)+1;
		}
		if(*p)
		{
			size_t l = _tcslen(p)+1;
			memcpy(p,p+l,(dwLsaBuf-((p+l)-lsaBuf))+1);
			dwLsaBuf-=l;
			if(RegSetValueEx(hk,_T("Authentication Packages"),NULL,dwType,(BYTE*)lsaBuf,dwLsaBuf))
			{
				fprintf(stderr,"Couldn't write LSA registry key, error %d\n",GetLastError());
				return -1;
			}
		}
		return 0;
	}

    if (!strcmp(argv[1],"-v"))
	{
        _putts(NTSERVICE_VERSION_STRING);
        return 0;
    }

	if(!strcmp(argv[1],"-i"))
	{
		HKEY hk;

		if(!CGlobalSettings::isAdmin())
		{
			fprintf(stderr,"Must be run as an administrator to use this option\n");
			return -1;
		}

		if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\CVS\\Pserver"),NULL,_T(""),REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hk,NULL))
		{ 
			fprintf(stderr,"Couldn't create HKLM\\Software\\CVS\\Pserver key, error %d\n",GetLastError());
			return -1;
		}

		if(argc==3)
		{
			szRoot = argv[2];
			if(GetFileAttributesA(szRoot)==(DWORD)-1)
			{
				fprintf(stderr,"Repository directory '%s' not found\n",szRoot);
				return -1;
			}
			dwType=REG_SZ;
			RegSetValueExA(hk,"Repository0",NULL,dwType,(BYTE*)szRoot,strlen(szRoot)+1);
		}
		// connect to  the service control manager
		if((hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE)) == NULL)
		{
			fprintf(stderr,"OpenSCManager Failed\n");
			return -1;
		}

		if((hService=OpenService(hSCManager,SERVICE_NAME,DELETE))!=NULL)
		{
			DeleteService(hService);
			CloseServiceHandle(hService);
		}

		GetModuleFileName(NULL,szImagePath,MAX_PATH);
		if ((hService = CreateService(hSCManager,SERVICE_NAME,DISPLAY_NAME,
						STANDARD_RIGHTS_REQUIRED|SERVICE_CHANGE_CONFIG, SERVICE_WIN32_OWN_PROCESS|SERVICE_INTERACTIVE_PROCESS,
						SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
						szImagePath, NULL, NULL, NULL, NULL, NULL)) == NULL)
		{
			fprintf(stderr,"CreateService Failed: %s\n",GetErrorString());
			return -1;
		}
		{
			BOOL (WINAPI *pChangeServiceConfig2)(SC_HANDLE,DWORD,LPVOID);
			pChangeServiceConfig2=(BOOL (WINAPI *)(SC_HANDLE,DWORD,LPVOID))GetProcAddress(GetModuleHandle(_T("advapi32")),"ChangeServiceConfig2A");
			if(pChangeServiceConfig2)
			{
				SERVICE_DESCRIPTION sd = { NTSERVICE_VERSION_STRING };
				if(!pChangeServiceConfig2(hService,SERVICE_CONFIG_DESCRIPTION,&sd))
				{
					0;
				}
				SERVICE_REQUIRED_PRIVILEGES_INFO sp = { SE_NETWORK_LOGON_NAME _T("\0") SE_IMPERSONATE_NAME _T("\0") };
				if(!pChangeServiceConfig2(hService,SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO,&sp))
				{
					0;
				}
			}
		}
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCManager);
		ReportError(FALSE,DISPLAY_NAMEA " installed successfully");
		printf(DISPLAY_NAMEA " installed successfully\n");
		RegCloseKey(hk);
	}
	

	if(!strcmp(argv[1],"-u"))
	{
		if(!CGlobalSettings::isAdmin())
		{
			fprintf(stderr,"Must be run as an administrator to use this option\n");
			return -1;
		}

		// connect to  the service control manager
		if((hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE)) == NULL)
		{
			fprintf(stderr,"OpenSCManager Failed\n");
			return -1;
		}

		if((hService=OpenService(hSCManager,SERVICE_NAME,DELETE))==NULL)
		{
			fprintf(stderr,"OpenService Failed: %s\n",GetErrorString());
			return -1;
		}
		if(!DeleteService(hService))
		{
			fprintf(stderr,"DeleteService Failed: %s\n",GetErrorString());
			return -1;
		}
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCManager);
		ReportError(FALSE,DISPLAY_NAMEA " uninstalled successfully");
		printf(DISPLAY_NAMEA " uninstalled successfully\n");
	}	
	else if(!strcmp(argv[1],"-test"))
	{
		if(!CGlobalSettings::isAdmin())
			fprintf(stderr,"**WARNING** Not running as administrator.  Some functions may fail.\n");

		ServiceMain(999,NULL);
	}
	else if(!strcmp(argv[1],"-sendstats"))
	{
		g_bTestMode=true;
		printf("Sending Statistics\n");
		SendStatistics();
	}
	return 0;
}

void CALLBACK ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	TCHAR szTmp[8192];
	char szTmpA[8192];
	TCHAR szTmp2[8192];
	char szAuthServer[32], szUnisonServer[32];
	DWORD dwTmp,dwType;
	HKEY hk,hk_env;
	int seq=1;
	LPCSTR szNode;
	int authserver_port = 2401;
	int unison_port = 0;
	bool bUnison;

	if(dwArgc!=999)
	{
		if (!(g_hService = RegisterServiceCtrlHandler(SERVICE_NAME,ServiceHandler))) { ReportError(TRUE,"Unable to start "SERVICE_NAMEA" - RegisterServiceCtrlHandler failed"); return; }
		NotifySCM(SERVICE_START_PENDING, 0, seq++);
	}
	else
	{
		g_bTestMode=TRUE;
		printf(SERVICE_NAMEA" " CVSNT_PRODUCTVERSION_STRING " ("__DATE__") starting in test mode.\n");
	}

	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"),NULL,KEY_QUERY_VALUE,&hk_env))
	{ 
		ReportError(TRUE,"Unable to start "SERVICE_NAMEA" - Couldn't open environment key"); 
		if(!g_bTestMode)
			NotifySCM(SERVICE_STOPPED,0,0);
		return;
	}

	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\CVS\\Pserver"),NULL,KEY_QUERY_VALUE|KEY_SET_VALUE,&hk))
	{
		ReportError(TRUE,"Unable to start "SERVICE_NAMEA" - Couldn't open HKLM\\Software\\CVS\\Pserver key");
		if(!g_bTestMode)
			NotifySCM(SERVICE_STOPPED,0,0);
		return;
	}

	dwTmp=sizeof(szTmp);
	szTmp2[0]='\0';

	if(!RegQueryValueEx(hk,_T("InstallPath"),NULL,&dwType,(BYTE*)szTmp2,&dwTmp))
	{
		_tcscat(szTmp2,_T(";"));
	}

	dwTmp=sizeof(szTmp);
	if(!RegQueryValueEx(hk_env,_T("PATH"),NULL,&dwType,(BYTE*)szTmp,&dwTmp))
		ExpandEnvironmentStrings(szTmp,szTmp2+_tcslen(szTmp2),sizeof(szTmp2)-_tcslen(szTmp2));

	if(!*szTmp2)
	{
		ReportError(TRUE,"Unable to start "SERVICE_NAMEA" - No path");
		if(!g_bTestMode)
			NotifySCM(SERVICE_STOPPED,0,0);
		return;
	}

	SetEnvironmentVariable(_T("PATH"),szTmp2);

	RegCloseKey(hk_env);

	dwTmp=sizeof(szTmp);
	if(RegQueryValueEx(hk,_T("TempDir"),NULL,&dwType,(LPBYTE)szTmp,&dwTmp) &&
	   SHRegGetUSValue(_T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"),_T("TEMP"),NULL,(LPVOID)szTmp,&dwTmp,TRUE,NULL,0) &&
	   !GetEnvironmentVariable(_T("TEMP"),(LPTSTR)szTmp,sizeof(szTmp)) &&
	   !GetEnvironmentVariable(_T("TMP"),(LPTSTR)szTmp,sizeof(szTmp)))
	{
		_tcscpy(szTmp,_T("C:\\"));
	}

	SetEnvironmentVariable(_T("TEMP"),szTmp);
	SetEnvironmentVariable(_T("TMP"),szTmp);
	SetEnvironmentVariable(_T("HOME"),szTmp);
	if(g_bTestMode)
		_tprintf(_T("TEMP/TMP currently set to %s\n"),szTmp);

	dwTmp=sizeof(DWORD);
	if(!RegQueryValueEx(hk,_T("PServerPort"),NULL,&dwType,(BYTE*)szTmp,&dwTmp))
	{
		authserver_port=*(DWORD*)szTmp;
	}
	itoa(authserver_port,szAuthServer,10);

	bUnison = true;
	dwTmp=sizeof(DWORD);
	if(!RegQueryValueEx(hk,_T("EnableUnison"),NULL,&dwType,(BYTE*)szTmp,&dwTmp))
	{
		if(*(DWORD*)szTmp)
			bUnison=true;
		else
			bUnison=false;
	}

	unison_port = 0;
	dwTmp=sizeof(DWORD);
	if(!RegQueryValueEx(hk,_T("UnisonPort"),NULL,&dwType,(BYTE*)szTmp,&dwTmp))
	{
		unison_port=*(DWORD*)szTmp;
	}
	if(unison_port)
		itoa(unison_port,szUnisonServer,10);
	else
		bUnison=false;

	CSocketIO cvs_sock;
	CSocketIO unison_sock;
	TCHAR unison_path[MAX_PATH];

	if(FindExecutable(_T("unison.exe"),NULL,unison_path)>=(HINSTANCE)32)
	{
		if(g_bTestMode)
			printf("Unison is available\n");
		if(!bUnison)
			printf("Unison is disabled\n");
	}
	else
		bUnison=false;

// Initialisation
	if(!CSocketIO::init())
	{
		ReportError(TRUE,"WSAStartup failed: %s\n",cvs_sock.error());
		if(!g_bTestMode)
			NotifySCM(SERVICE_STOPPED,0,0);
		return;
	}

	dwTmp=sizeof(szTmpA);
	szNode = NULL;
	if(!RegQueryValueExA(hk,"BindAddress",NULL,&dwType,(BYTE*)szTmpA,&dwTmp))
	{
		if(stricmp(szTmpA,"*"))
			szNode = szTmpA;
	}

	if(g_bTestMode)
	{
		printf("Initialising socket...\n");
	}

	if(!cvs_sock.create(szNode,szAuthServer,false))
	{
		ReportError(FALSE,"Failed to create socket: %s\n",cvs_sock.error());
		if(g_bTestMode)
			printf("Failed to create socket: %s\n",cvs_sock.error());
		else
			NotifySCM(SERVICE_STOPPED,0,0);
		return;
	}

	if(bUnison && !unison_sock.create(szNode,szUnisonServer,false))
	{
		ReportError(FALSE,"Failed to create socket: %s\n",cvs_sock.error());
		if(g_bTestMode)
			printf("Failed to create socket: %s\n",cvs_sock.error());
		else
			NotifySCM(SERVICE_STOPPED,0,0);
		return;
	}

	if(!g_bTestMode)
		NotifySCM(SERVICE_START_PENDING, 0, seq++);

	if(g_bTestMode)
	{
		printf("Starting auth server on port %d/tcp...\n",authserver_port);
		if(bUnison)
			printf("Starting unison server on port %d/tcp...\n",unison_port);
	}


	if(!cvs_sock.bind())
	{
		ReportError(TRUE,"Failed to bind socket: %s\n",cvs_sock.error());
		if(g_bTestMode)
			printf("Failed to bind socket: %s\n",cvs_sock.error());
		else
			NotifySCM(SERVICE_STOPPED,0,0);
		return;
	}

	if(bUnison && !unison_sock.bind())
	{
		ReportError(TRUE,"Failed to bind socket: %s\n",cvs_sock.error());
		if(g_bTestMode)
			printf("Failed to bind socket: %s\n",cvs_sock.error());
		else
			NotifySCM(SERVICE_STOPPED,0,0);
		return;
	}

	if(LoadLibrary(_T("NtDsApi")))
	{
		CoInitialize(NULL);
		try
		{
			ActiveDs::IADsADSystemInfoPtr info(CLSID_ADSystemInfo);
			_bstr_t path(info->ComputerName);

			if(g_bTestMode)
			{
				printf("Registering service SPN...\n");
				printf("DSN=%s\n",(const char *)path);
			}

			DWORD nspn = 0;
			LPTSTR *pspn;
			HANDLE hDS = NULL;
			dwTmp = DsBind(NULL,NULL,&hDS);
			if(!dwTmp)
				dwTmp = DsGetSpn(DS_SPN_DNS_HOST,_T("cvs"),NULL,0,0,NULL,NULL,&nspn,&pspn);
			if(g_bTestMode && nspn)
				printf("Adding %s\n",pspn[0]);
			if(!dwTmp)
				dwTmp = DsWriteAccountSpn(hDS,DS_SPN_ADD_SPN_OP ,path,nspn,(LPCTSTR*)pspn);
			if(nspn)
				DsFreeSpnArray(nspn,pspn);
			nspn = 0;
			if(!dwTmp)
				dwTmp = DsGetSpn(DS_SPN_NB_HOST,_T("cvs"),NULL,0,0,NULL,NULL,&nspn,&pspn);
			if(g_bTestMode && nspn)
				printf("Adding %s\n",pspn[0]);
			if(!dwTmp)
				dwTmp = DsWriteAccountSpn(hDS,DS_SPN_ADD_SPN_OP,path,nspn,(LPCTSTR*)pspn);
			if(nspn)
				DsFreeSpnArray(nspn,pspn);
			if(hDS)
				dwTmp = DsUnBind(&hDS);

			if(dwTmp)
			{
				if(g_bTestMode)
					printf("Active Directory registration failed (Error %d)\n",dwTmp);
			}
		}
		catch(_com_error e)
		{
			if(g_bTestMode)
				printf("Couldn't register with active directory: %S\n",e.ErrorMessage());
		}
	}

	// Process running, wait for closedown
	ReportError(FALSE,SERVICE_NAMEA" initialised successfully");
	if(!g_bTestMode)
		NotifySCM(SERVICE_RUNNING, 0, 0);

	servtime_t servtime;
	_time64(&servtime.t);
	RegSetValueEx(hk,_T("StartTime"),NULL,REG_BINARY,servtime.data,sizeof(servtime.data));

	dwTmp=sizeof(g_dwMaxUsers);
	if(RegQueryValueEx(hk,_T("MaxUsers"),NULL,&dwType,(LPBYTE)&g_dwMaxUsers,&dwTmp) || dwType!=REG_DWORD)
		g_dwMaxUsers=0;
	
	dwTmp=sizeof(g_dwAverageTime);
	if(RegQueryValueEx(hk,_T("AverageTime"),NULL,&dwType,(LPBYTE)&g_dwAverageTime,&dwTmp) || dwType!=REG_DWORD)
		g_dwAverageTime=0;

	dwTmp=sizeof(g_dwSessionCount);
	if(RegQueryValueEx(hk,_T("SessionCount"),NULL,&dwType,(LPBYTE)&g_dwSessionCount,&dwTmp) || dwType!=REG_DWORD)
		g_dwSessionCount=0;

	g_dwUsers=0;
	g_bStop=FALSE;

	InitServer();

	DWORD dwOldMaxUsers = g_dwMaxUsers;

	CSocketIO* sock_list[2] = { &cvs_sock, &unison_sock };
	
	while(!g_bStop && CSocketIO::select(5000,bUnison?2:1,sock_list))
	{
		for(size_t n=0; n<cvs_sock.accepted_sockets().size(); n++)
			CloseHandle(CreateThread(NULL,0,DoCvsThread,(void*)cvs_sock.accepted_sockets()[n].Detach(),0,NULL));
		if(bUnison)
		{
			for(size_t n=0; n<unison_sock.accepted_sockets().size(); n++)
				CloseHandle(CreateThread(NULL,0,DoUnisonThread,(void*)unison_sock.accepted_sockets()[n].Detach(),0,NULL));
		}
		if(dwOldMaxUsers<g_dwMaxUsers)
		{
			dwOldMaxUsers=g_dwMaxUsers;
			RegSetValueEx(hk,_T("MaxUsers"),NULL,REG_DWORD,(LPBYTE)&dwOldMaxUsers,sizeof(dwOldMaxUsers));
		}
		RegSetValueEx(hk,_T("SessionCount"),NULL,REG_DWORD,(LPBYTE)&g_dwSessionCount,sizeof(g_dwSessionCount));
		RegSetValueEx(hk,_T("AverageTime"),NULL,REG_DWORD,(LPBYTE)&g_dwAverageTime,sizeof(g_dwAverageTime));

		DWORD dwSendStatistics=0;
		dwTmp=sizeof(dwSendStatistics);
		if(RegQueryValueEx(hk,_T("SendStatistics"),NULL,&dwType,(LPBYTE)&dwSendStatistics,&dwTmp) || dwType!=REG_DWORD)
			dwSendStatistics=1;

		if (dwSendStatistics)
		{
		servtime_t lastAttempt,lastSuccess;
		dwTmp=sizeof(lastAttempt.data);
		if(RegQueryValueEx(hk,_T("LastAttemptedSend"),NULL,&dwType,(LPBYTE)&lastAttempt.data,&dwTmp) || dwType!=REG_BINARY)
			lastAttempt.t=0;
		dwTmp=sizeof(lastSuccess.data);
		if(RegQueryValueEx(hk,_T("LastSend"),NULL,&dwType,(LPBYTE)&lastSuccess.data,&dwTmp) || dwType!=REG_BINARY)
			lastSuccess.t=0;
		__time64_t nextAttempt,now;
		if(lastAttempt.t==lastSuccess.t)
			nextAttempt = lastAttempt.t + 604800; // 1 week
		else
			nextAttempt = lastAttempt.t + 28800; // 8 hour retry on failure
		
		_time64(&now);
		if(nextAttempt<=now)
		{
			if(g_bTestMode)
				printf("Sending Statistics\n");
			bool bSuccess = SendStatistics();
			if(g_bTestMode)
				printf("Sending Statistics %s\n",bSuccess?"succeeded":"failed");
			
			lastAttempt.t=now;
			RegSetValueEx(hk,_T("LastAttemptedSend"),NULL,REG_BINARY,(LPBYTE)&lastAttempt.data,sizeof(lastAttempt.data));
			if(bSuccess)
				RegSetValueEx(hk,_T("LastSend"),NULL,REG_BINARY,(LPBYTE)&lastAttempt.data,sizeof(lastAttempt.data));
		}
		} // dwSendStatistics
	}

	NotifySCM(SERVICE_STOPPED, 0, 0);
	ReportError(FALSE,SERVICE_NAMEA" stopped successfully");

	RegSetValueEx(hk,_T("StartTime"),NULL,REG_BINARY,(LPBYTE)"",0);
	RegCloseKey(hk);
}

void CALLBACK ServiceHandler(DWORD fdwControl)
{
	switch(fdwControl)
	{      
	case SERVICE_CONTROL_STOP:
		OutputDebugString(SERVICE_NAME _T(": Stop\n"));
		NotifySCM(SERVICE_STOP_PENDING, 0, 0);
		g_bStop=TRUE;
		return;
	case SERVICE_CONTROL_INTERROGATE:
	default:
		break;
	}
	OutputDebugString(SERVICE_NAME _T(": Interrogate\n"));
	NotifySCM(g_dwCurrentState, 0, 0);
}

BOOL NotifySCM(DWORD dwState, DWORD dwWin32ExitCode, DWORD dwProgress)
{
	SERVICE_STATUS ServiceStatus;

	// fill in the SERVICE_STATUS structure
	ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ServiceStatus.dwCurrentState = g_dwCurrentState = dwState;
	ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	ServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
	ServiceStatus.dwServiceSpecificExitCode = 0;
	ServiceStatus.dwCheckPoint = dwProgress;
	ServiceStatus.dwWaitHint = 3000;

	// send status to SCM
	return SetServiceStatus(g_hService, &ServiceStatus);
}

char* basename(const char* str)
{
	char*p = ((char*)str)+strlen(str)-1;
	while(p>str && *p!='\\')
		p--;
	if(p>str) return (p+1);
	else return p;
}

LPCSTR GetErrorString()
{
	static char ErrBuf[1024];

	FormatMessageA(
    FORMAT_MESSAGE_FROM_SYSTEM |
	FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
    (LPSTR) ErrBuf,
    sizeof(ErrBuf),
    NULL );
	return ErrBuf;
};

void ReportError(BOOL bError, LPCSTR szError, ...)
{
	static BOOL bEventSourceAdded = FALSE;
	char buf[512];
	const char *bufp = buf;
	va_list va;

	va_start(va,szError);
	vsprintf(buf,szError,va);
	va_end(va);
	if(g_bTestMode)
	{
		printf("%s%s\n",bError?"Error: ":"",buf);
	}
	else
	{
		if(!bEventSourceAdded)
		{
			TCHAR szModule[MAX_PATH];
			GetModuleFileName(NULL,szModule,MAX_PATH);
			AddEventSource(SERVICE_NAME,szModule);
			bEventSourceAdded=TRUE;
		}

		HANDLE hEvent = RegisterEventSource(NULL,  SERVICE_NAME);
		ReportEventA(hEvent,bError?EVENTLOG_ERROR_TYPE:EVENTLOG_INFORMATION_TYPE,0,MSG_STRING,NULL,1,0,&bufp,NULL);
		DeregisterEventSource(hEvent);
	}
}

void AddEventSource(LPCTSTR szService, LPCTSTR szModule)
{
	HKEY hk;
	DWORD dwData;
	TCHAR szKey[1024];

	_tcscpy(szKey,_T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\"));
	_tcscat(szKey,szService);

    // Add your source name as a subkey under the Application 
    // key in the EventLog registry key.  
    if (RegCreateKey(HKEY_LOCAL_MACHINE, szKey, &hk))
		return; // Fatal error, no key and no way of reporting the error!!!

    // Add the name to the EventMessageFile subkey.  
    if (RegSetValueEx(hk,             // subkey handle 
            _T("EventMessageFile"),       // value name 
            0,                        // must be zero 
            REG_EXPAND_SZ,            // value type 
            (LPBYTE) szModule,           // pointer to value data 
            _tcslen(szModule) + 1))       // length of value data 
			return; // Couldn't set key

    // Set the supported event types in the TypesSupported subkey.  
    dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | 
        EVENTLOG_INFORMATION_TYPE;  
    if (RegSetValueEx(hk,      // subkey handle 
            _T("TypesSupported"),  // value name 
            0,                 // must be zero 
            REG_DWORD,         // value type 
            (LPBYTE) &dwData,  // pointer to value data 
            sizeof(DWORD)))    // length of value data 
			return; // Couldn't set key
	RegCloseKey(hk); 
} 

DWORD CALLBACK DoCvsThread(LPVOID lpParam)
{
	CSocketIOPtr conn = (CSocketIO*)lpParam;
	CRunFile rf;
	cvs::string line;
	DWORD timestart = GetTickCount(),timeend;

	InterlockedIncrement(&g_dwUsers);
	if(g_dwUsers>g_dwMaxUsers) g_dwMaxUsers=g_dwUsers;

	cvs::sprintf(line,128,"cvs.exe --win32_socket_io=%ld authserver",(long)conn->getsocket());
	rf.setArgs(line.c_str());
	rf.run(NULL);
	if(g_bTestMode)
		printf("%08x: Cvsnt process started\n",GetTickCount());
	do
	{
		int ret;
		if(rf.wait(ret,5000))
			break;
	} while(!g_bStop);

	InterlockedDecrement(&g_dwUsers);
	timeend = GetTickCount();

	InterlockedIncrement(&g_dwSessionCount);

	if(timeend>timestart)
	{
		ClientLock lock;

		// There must be a better way than multiply/add/divide but I can't think of it right now

		DWORD sessiontime = timeend - timestart;
		__int64 tot = g_dwAverageTime;
		tot *= (g_dwSessionCount-1);
		tot += sessiontime;
		tot /= g_dwSessionCount;
		g_dwAverageTime = (DWORD)tot;
	}

	if(g_bStop)
		rf.terminate();

	if(g_bTestMode)
		printf("%08x: Cvsnt process %terminated\n",GetTickCount());

	return 0;
}

DWORD CALLBACK DoUnisonThread(LPVOID lpParam)
{
	CSocketIOPtr conn = (CSocketIO*)lpParam;
	CRunFile rf;
	cvs::string line;

	cvs::sprintf(line,128,"unison.exe -server");
	rf.setArgs(line.c_str());

	rf.setInput(CRunFile::StandardInput,NULL);
	rf.setOutput(CRunFile::StandardOutput,NULL);
	rf.setError(CRunFile::StandardError,NULL);

	SetStdHandle(STD_INPUT_HANDLE,(HANDLE)conn->getsocket());
	SetStdHandle(STD_OUTPUT_HANDLE,(HANDLE)conn->getsocket());
	SetStdHandle(STD_ERROR_HANDLE,(HANDLE)conn->getsocket());

	rf.run(NULL);
	if(g_bTestMode)
		printf("%08x: Unison process started\n",GetTickCount());
	do
	{
		int ret;
		if(rf.wait(ret,5000))
			break;
	} while(!g_bStop);

	if(g_bStop)
		rf.terminate();

	if(g_bTestMode)
		printf("%08x: Unison process %terminated\n",GetTickCount());

	return 0;
}

bool SendStatistics()
{
	CHttpSocket sock;
	TCHAR os[256],servicepack[256];
	servtime_t servtime;
	__time64_t uptime;
	DWORD dwLen,dwType;
	DWORD dwMaxUsers;
	DWORD dwAverageTime;
	DWORD dwSessionCount;
	DWORD dwUserCount;
	DWORD dwCodepage;
	DWORD dwMajor, dwMinor;
	cvs::string registration;
	HKEY hInstallerKey,hServerKey;

	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\CVS\\Pserver"),0,KEY_READ,&hServerKey))
		return false;

	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\March Hare Software Ltd\\Keys"),0,KEY_READ,&hInstallerKey))
	{
		RegCloseKey(hServerKey);
		return false;
	}

	dwCodepage = GetACP();

	GetOsVersion(os,servicepack, dwMajor, dwMinor);

	dwLen=sizeof(servtime.data);
	if(!RegQueryValueEx(hServerKey,_T("StartTime"),NULL,&dwType,servtime.data,&dwLen) && dwType==REG_BINARY)
	{
		__time64_t then = servtime.t,now;
		_time64(&now);
		if(then>now) uptime=0;
		else uptime=now-then;
	}
	else uptime=0;

	dwLen=sizeof(dwMaxUsers);
	if(!RegQueryValueEx(hServerKey,_T("MaxUsers"),NULL,&dwType,(LPBYTE)&dwMaxUsers,&dwLen) && dwType==REG_DWORD)
		;
	else
		dwMaxUsers=0;
		
	dwLen=sizeof(dwAverageTime);
	if(!RegQueryValueEx(hServerKey,_T("AverageTime"),NULL,&dwType,(LPBYTE)&dwAverageTime,&dwLen) && dwType==REG_DWORD)
		;
	else
		dwAverageTime=0;
		
	dwLen=sizeof(dwSessionCount);
	if(!RegQueryValueEx(hServerKey,_T("SessionCount"),NULL,&dwType,(LPBYTE)&dwSessionCount,&dwLen) && dwType==REG_DWORD)
		;
	else
		dwSessionCount=0;
		
	dwLen=sizeof(dwUserCount);
	if(!RegQueryValueEx(hServerKey,_T("UserCount"),NULL,&dwType,(LPBYTE)&dwUserCount,&dwLen) && dwType==REG_DWORD)
		;
	else
		dwUserCount=0;
		
	registration.resize(256);
	dwLen=registration.size();
	if(!RegQueryValueExA(hInstallerKey,"serversuite",NULL,&dwType,(LPBYTE)registration.data(),&dwLen) || dwType!=REG_SZ)
	{
		dwLen=registration.size();
		if(!RegQueryValueExA(hInstallerKey,"server",NULL,&dwType,(LPBYTE)registration.data(),&dwLen) || dwType!=REG_SZ)
		{
			dwLen=registration.size();
			if(!RegQueryValueExA(hInstallerKey,"servertrial",NULL,&dwType,(LPBYTE)registration.data(),&dwLen) || dwType!=REG_SZ)
				registration="";
			else
				registration.resize(dwLen);
		}
		else
			registration.resize(dwLen);
	}
	else
		registration.resize(dwLen);

	RegCloseKey(hServerKey);
	RegCloseKey(hInstallerKey);

	cvs::string url;
	cvs::sprintf(url,128,"/cvspro/prods-pre.asp?register=stats&os=%s&sp=%s&ver=%s&avgtime=%u&up=%u&users=%u&uusers=%u&susers=%u&id=%s&cp=%u&major=%u&minor=%u",
		(const char *)cvs::narrow((const wchar_t *)os),(const char *)cvs::narrow((const wchar_t *)servicepack),CVSNT_PRODUCTVERSION_SHORT,dwAverageTime,(DWORD)uptime,dwSessionCount,dwUserCount,dwMaxUsers,registration.c_str(),dwCodepage,dwMajor,dwMinor);

	// To be 100% certain, replace all ' ' with %20.  Webservers can cope but there may be a proxy
	// that doesn't.

	size_t pos;
	while((pos=url.find_first_of(' '))!=cvs::string::npos)
		url.replace(pos,1,"%20");

	if(!sock.create("http://march-hare.com"))
	{
		if(g_bTestMode)
			printf("Failed to send statistics.  Server not responding\n");
		return false;
	}

	if(!sock.request("GET",url.c_str()))
	{
		// Not sure this can actually fail.. it would normally succeed with an error code
		if(g_bTestMode)
			printf("Failed to send statistics.  Server not responding\n");
		return false;
	}

	if(sock.responseCode()!=200)
	{
		if(g_bTestMode)
			printf("Failed to send statistics.  Server returned error %d\n",sock.responseCode());
		return false;
	}

	return true;
}

// From MSDN
void GetOsVersion(LPTSTR os, LPTSTR servicepack, DWORD& dwMajor, DWORD& dwMinor)
{
	DWORD dwType;

	OSVERSIONINFOEX vi = {sizeof(OSVERSIONINFOEX)};
	GetVersionEx((OSVERSIONINFO*)&vi);

	SYSTEM_INFO si = {0};
	PGNSI pGNSI = (PGNSI)GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "GetNativeSystemInfo");
	if(pGNSI) pGNSI(&si);
	else GetSystemInfo(&si);

	dwMajor = vi.dwMajorVersion;
	dwMinor = vi.dwMinorVersion;

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

	lstrcpy(servicepack,vi.szCSDVersion);
}
