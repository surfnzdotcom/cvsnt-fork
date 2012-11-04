// installer.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "../../version.h"

static const TCHAR *register_url = _T("http://march-hare.com/cvspro/prods-pre.asp");

struct __parm
{
	__parm(bool b) { type = 0; }
	__parm(LPCTSTR f) { type=1; pf = f; }
	__parm(UINT f) { type=2; uf = f; }
	int type;
	LPCTSTR pf;
	UINT uf;

	void set(MSIHANDLE hRecord, UINT field)
	{ 
		switch(type)
		{
		case 1: MsiRecordSetString(hRecord,field,pf); break;
		case 2: MsiRecordSetInteger(hRecord,field,uf); break;
		}
	}
};

void Log(MSIHANDLE hInstall, LPCTSTR f1, __parm f2 = false, __parm f3 = false, __parm f4 = false)
{
	PMSIHANDLE hRecord = MsiCreateRecord(4);
	MsiRecordSetString(hRecord,0,f1);
	f2.set(hRecord,1);
	f3.set(hRecord,2);
	f4.set(hRecord,3);
	MsiProcessMessage(hInstall,INSTALLMESSAGE(INSTALLMESSAGE_INFO),hRecord);
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}

static void GetOsVersion(LPTSTR os, LPTSTR servicepack)
{
	typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
	typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);

#ifndef PRODUCT_ULTIMATE
// It's truly amazing how many 'editions' of windows there are..
#define PRODUCT_BUSINESS	0x00000006
#define PRODUCT_BUSINESS_N	0x00000010
#define PRODUCT_CLUSTER_SERVER 0x00000012
#define PRODUCT_DATACENTER_SERVER 0x00000008
#define PRODUCT_DATACENTER_SERVER_CORE 0x0000000C
#define PRODUCT_ENTERPRISE 0x00000004
#define PRODUCT_ENTERPRISE_N 0x0000001B
#define PRODUCT_ENTERPRISE_SERVER 0x0000000A
#define PRODUCT_ENTERPRISE_SERVER_CORE 0x0000000E
#define PRODUCT_ENTERPRISE_SERVER_IA64 0x0000000F
#define PRODUCT_HOME_BASIC 0x00000002
#define PRODUCT_HOME_BASIC_N 0x00000005
#define PRODUCT_HOME_PREMIUM 0x00000003
#define PRODUCT_HOME_PREMIUM_N 0x0000001A
#define PRODUCT_HOME_SERVER 0x00000013
#define PRODUCT_SERVER_FOR_SMALLBUSINESS 0x00000018
#define PRODUCT_SMALLBUSINESS_SERVER 0x00000009
#define PRODUCT_SMALLBUSINESS_SERVER_PREMIUM 0x00000019
#define PRODUCT_STANDARD_SERVER 0x00000007
#define PRODUCT_STANDARD_SERVER_CORE 0x0000000D
#define PRODUCT_STARTER 0x0000000B
#define PRODUCT_STORAGE_ENTERPRISE_SERVER 0x00000017
#define PRODUCT_STORAGE_EXPRESS_SERVER 0x00000014
#define PRODUCT_STORAGE_STANDARD_SERVER 0x00000015
#define PRODUCT_STORAGE_WORKGROUP_SERVER 0x00000016
#define PRODUCT_UNDEFINED 0x00000000
#define PRODUCT_ULTIMATE 0x00000001
#define PRODUCT_ULTIMATE_N 0x0000001C
#define PRODUCT_WEB_SERVER 0x00000011
#define PRODUCT_WEB_SERVER_CORE 0x0000001D

// Plus a special flag to GetSystemMetrics just for Win2003 R2
#define SM_SERVERR2 89

#endif
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

	  lstrcpy(servicepack, vi.szCSDVersion);
   }
}

static bool PostInstallData(MSIHANDLE hInstall, bool allusers,LPCTSTR url, LPCTSTR package, bool install, LPCTSTR serial, LPCTSTR os, LPCTSTR servicepack, LPCTSTR version)
{
	HINTERNET hInternet = WinHttpOpen(_T("March Hare Installer"), WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if(hInternet)
	{
		WINHTTP_AUTOPROXY_OPTIONS apo = {0};
		WINHTTP_PROXY_INFO api = {0};
		apo.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
		apo.dwAutoDetectFlags = /*WINHTTP_AUTO_DETECT_TYPE_DHCP|*/WINHTTP_AUTO_DETECT_TYPE_DNS_A;
		apo.fAutoLogonIfChallenged = TRUE;
		if(WinHttpGetProxyForUrl(hInternet, url, &apo, &api))
		{
			WinHttpSetOption(hInternet,WINHTTP_OPTION_PROXY,&api,sizeof(api));
			if(api.lpszProxy) GlobalFree(GlobalHandle(api.lpszProxy));
			if(api.lpszProxyBypass) GlobalFree(GlobalHandle(api.lpszProxyBypass));
		}
		else
		{
			WINHTTP_CURRENT_USER_IE_PROXY_CONFIG iepc={0};
			if(WinHttpGetIEProxyConfigForCurrentUser(&iepc))
			{
				if(!iepc.fAutoDetect && iepc.lpszProxy)
				{
					api.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
					api.lpszProxy = iepc.lpszProxy;
					api.lpszProxyBypass = iepc.lpszProxyBypass;
					WinHttpSetOption(hInternet,WINHTTP_OPTION_PROXY,&api,sizeof(api));
				}
				if(iepc.lpszProxy) GlobalFree(GlobalHandle(iepc.lpszProxy));
				if(iepc.lpszProxyBypass) GlobalFree(GlobalHandle(iepc.lpszProxyBypass));
				if(iepc.lpszAutoConfigUrl) GlobalFree(GlobalHandle(iepc.lpszAutoConfigUrl));
			}
		}

		URL_COMPONENTS urlc = { sizeof(urlc) };
		TCHAR szHostName[64];
		TCHAR szExtraInfo[256];
		DWORD dw;

		urlc.dwSchemeLength=-1;
		urlc.dwHostNameLength=-1;
		urlc.dwUrlPathLength=-1;
		urlc.dwExtraInfoLength=-1;

		if(!WinHttpCrackUrl(url,0,0,&urlc))
			dw = GetLastError();

		lstrcpyn(szHostName,urlc.lpszHostName,urlc.dwHostNameLength+1);
		szHostName[urlc.dwHostNameLength+1]='\0';

		HINTERNET hSession = WinHttpConnect(hInternet, szHostName, urlc.nPort,  0);
		if(hSession)
		{
			TCHAR newurl[256];
			DWORD dwurlLen=sizeof(newurl)/sizeof(newurl[0]);
			if(install)
				_sntprintf(szExtraInfo,sizeof(szExtraInfo)/sizeof(szExtraInfo[0]),_T("?register=%s&os=%s&sp=%s&ver=%s"),package,os,servicepack,version);
			else
				_sntprintf(szExtraInfo,sizeof(szExtraInfo)/sizeof(szExtraInfo[0]),_T("?register=remove&id=%s"),serial);
			urlc.lpszExtraInfo = szExtraInfo;
			urlc.dwExtraInfoLength=lstrlen(szExtraInfo);
			if(!WinHttpCreateUrl(&urlc,ICU_ESCAPE,newurl,&dwurlLen))
				dw = GetLastError();
			memset(&urlc,0,sizeof(urlc));
			urlc.dwStructSize=sizeof(urlc);
			urlc.dwUrlPathLength = -1;
			if(!WinHttpCrackUrl(newurl,0,0,&urlc))
				dw = GetLastError();

			HINTERNET hRequest = WinHttpOpenRequest(hSession, _T("GET"),  urlc.lpszUrlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
			if(hRequest)
			{
				LPTSTR szOptionalData = 0;
				DWORD dwOptionalLength = 0;
				if(WinHttpSendRequest(hRequest,WINHTTP_NO_ADDITIONAL_HEADERS,0,szOptionalData,dwOptionalLength,dwOptionalLength,NULL))
				{
					if(WinHttpReceiveResponse(hRequest,NULL))
					{
						DWORD dwSize;
						if(WinHttpQueryDataAvailable(hRequest,&dwSize))
						{
							LPBYTE lpBuf = new BYTE[dwSize+1];
							lpBuf[dwSize]='\0';
							if(WinHttpReadData(hRequest,lpBuf,dwSize,NULL))
							{
								if(install)
								{
									// Stick serial in the registry, theoretically
									const char *p = strstr((const char *)lpBuf,"number:");
									if(p)
									{
										p+=7;
										const char *q=strchr(p,'\n');
										if(!q)
											q=p+strlen(p);
										TCHAR serial[256];
										HKEY hKey;
										serial[MultiByteToWideChar(CP_UTF8,0,p,(int)(q-p),serial,sizeof(serial)/sizeof(serial[0]))]='\0';
										Log(hInstall,_T("Serial is [1]"),serial);
										if(!RegCreateKey(allusers?HKEY_LOCAL_MACHINE:HKEY_CURRENT_USER,_T("Software\\March Hare Software Ltd\\Keys"),&hKey))
										{
											RegSetValueEx(hKey,package,NULL,REG_SZ,(BYTE*)serial,(lstrlen(serial)+1)*sizeof(serial[0]));
											RegCloseKey(hKey);
										}
										else
											Log(hInstall,_T("Couldn't create registry key"));
									}
								}
								else /* uninstall */
								{
									HKEY hKey;
									if(!RegOpenKey(allusers?HKEY_LOCAL_MACHINE:HKEY_CURRENT_USER,_T("Software\\March Hare Software Ltd\\Keys"),&hKey))
									{
										RegDeleteValue(hKey,package);
										RegCloseKey(hKey);
									}
								}
							}
							else
								Log(hInstall,_T("WinHttpReadData failed"));
							delete[] lpBuf;
						}
						else
							Log(hInstall,_T("WinHttpQueryDataAvailable failed"));
					}
					else
						Log(hInstall,_T("WinHttpReceiveResponse failed"));
				}
				else
					Log(hInstall,_T("WinHttpSendRequest failed"));
				WinHttpCloseHandle(hRequest);
			}
			else
				Log(hInstall,_T("WinHttpOpenRequest failed"));
			WinHttpCloseHandle(hSession);
		}
		else
			Log(hInstall,_T("WinHttpConnect failed"));

		WinHttpCloseHandle(hInternet);
	}
	else
		Log(hInstall,_T("Initial WinHttpOpen failed"));
	return true;
}

UINT WINAPI CustomActionInstall(MSIHANDLE hInstall)
{
	TCHAR type[64],szallusers[64];
	DWORD dwSize = sizeof(type)/sizeof(type[0]);
	bool allusers;

	Log(hInstall,_T("installer.dll - Install"));

	UINT dwErr = MsiGetProperty(hInstall, _T("CVSNT_INSTALLTYPE"), type, &dwSize);
	if(dwErr)
	{
		// One option here is to just barf, but that'll lead to it not getting registered, better to 
		// make some sort of assumption
		lstrcpy(type,_T("server"));
	}
	dwSize = sizeof(szallusers)/sizeof(szallusers[0]);
	dwErr = MsiGetProperty(hInstall, _T("ALLUSERS"), szallusers, &dwSize);
	if(dwErr)
	{
		// One option here is to just barf, but that'll lead to it not getting registered, better to 
		// make some sort of assumption
		lstrcpy(szallusers,_T("0"));
	}
	allusers=_ttoi(szallusers)?true:false;
	if(!lstrlen(type))
	{
		Log(hInstall,_T("CVSNT_INSTALLTYPE is not set"));
		return 0;
	}

	Log(hInstall,_T("CVSNT_INSTALLTYPE is [1]"),type);

	CoInitialize(NULL);

	TCHAR os[128];
	TCHAR servicepack[128];

	GetOsVersion(os,servicepack);

	Log(hInstall,_T("OS Version is [1], ServicePack is [2]"),os,servicepack);
	
	PostInstallData(hInstall,allusers,register_url, type, true, NULL, os, servicepack, _T( CVSNT_PRODUCTVERSION_SHORT ));

	return 0;
}

UINT WINAPI CustomActionUninstall(MSIHANDLE hInstall)
{
	TCHAR type[64],szallusers[64];
	DWORD dwSize = sizeof(type)/sizeof(type[0]);
	bool allusers;

	Log(hInstall,_T("installer.dll - Uninstall"));

	UINT dwErr = MsiGetProperty(hInstall, _T("CVSNT_INSTALLTYPE"), type, &dwSize);
	if(dwErr)
	{
		// One option here is to just barf, but that'll lead to it not getting registered, better to 
		// make some sort of assumption
		lstrcpy(type,_T("server"));
	}
	dwSize = sizeof(szallusers)/sizeof(szallusers[0]);
	dwErr = MsiGetProperty(hInstall, _T("ALLUSERS"), szallusers, &dwSize);
	if(dwErr)
	{
		// One option here is to just barf, but that'll lead to it not getting registered, better to 
		// make some sort of assumption
		lstrcpy(szallusers,_T("0"));
	}
	allusers=_ttoi(szallusers)?true:false;
	if(!lstrlen(type))
	{
		Log(hInstall,_T("CVSNT_INSTALLTYPE is not set"));
		return 0;
	}

	Log(hInstall,_T("CVSNT_INSTALLTYPE is [1]"),type);

	CoInitialize(NULL);

	TCHAR serial[256];
	DWORD dwSerial = sizeof(serial);
	HKEY hKey;
	if(!RegOpenKey(allusers?HKEY_LOCAL_MACHINE:HKEY_CURRENT_USER,_T("Software\\March Hare Software Ltd\\Keys"),&hKey))
	{
		if(RegQueryValueEx(hKey,type,NULL,NULL,(LPBYTE)serial,&dwSerial))
			lstrcpy(serial,_T("Unknown"));
		RegCloseKey(hKey);
	}
	else
		lstrcpy(serial,_T("Unknown"));

	PostInstallData(hInstall,allusers,register_url, type, false,serial, NULL, NULL, _T( CVSNT_PRODUCTVERSION_SHORT ));

	return 0;
}
