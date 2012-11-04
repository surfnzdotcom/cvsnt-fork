/*
	CVSNT Generic API
    Copyright (C) 2005 Tony Hoyle and March-Hare Software Ltd

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
// Db2
// Win32 specific - could be ported to unix in theory
#ifdef _WIN32
// Microsoft braindamage reversal.  
#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_WARNINGS
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <config.h>
#include <cvsapi.h>

#include <sql.h>
#include <sqlext.h>

#include "Db2Connection.h"
#include "Db2Recordset.h"
#include "Db2ConnectionInformation.h"

#ifdef _WIN32
  #define SQL_EXPORT __declspec(dllexport)
#elif defined(HAVE_GCC_VISIBILITY)
  #define SQL_EXPORT __attribute__ ((visibility("default")))
#else
  #define SQL_EXPORT 
#endif

extern "C" SQL_EXPORT CSqlConnection *CreateConnection()
{
        return new CDb2Connection;
}

#undef SQL_EXPORT

CDb2Connection::CDb2Connection()
{
	m_hEnv=NULL;
	m_hDbc=NULL;
	m_lasterror=SQL_SUCCESS;
}

CDb2Connection::~CDb2Connection()
{
	Close();
}

bool CDb2Connection::Create(const char *host, const char *database, const char *username, const char *password)
{
	CDb2ConnectionInformation *pCI = static_cast<CDb2ConnectionInformation*>(GetConnectionInformation());
	pCI->hostname=host?host:"localhost";
	pCI->database=database?database:"";
	pCI->username=username?username:"";
	pCI->password=password?password:"";

	return Create();
}

bool CDb2Connection::Create()
{
	CDb2ConnectionInformation *pCI = static_cast<CDb2ConnectionInformation*>(GetConnectionInformation());
	cvs::string db = pCI->database;
	pCI->database="system";  // fixme.. how do you open a nonspecific database in db2?
	if(!Open())
		return false;
	pCI->database=db;
	Execute("create database %s",db.c_str());
	if(Error())
		return false;
	Close();
	return Open();
}

bool CDb2Connection::Open(const char *host, const char *database, const char *username, const char *password)
{
	CDb2ConnectionInformation *pCI = static_cast<CDb2ConnectionInformation*>(GetConnectionInformation());
	pCI->hostname=host?host:"localhost";
	pCI->database=database?database:"";
	pCI->username=username?username:"";
	pCI->password=password?password:"";

	return Open();
}

bool CDb2Connection::Open()
{
	CDb2ConnectionInformation *pCI = static_cast<CDb2ConnectionInformation*>(GetConnectionInformation());

	m_lasterror = SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&m_hEnv);
	if(!SQL_SUCCEEDED(m_lasterror))
		return false;

	m_lasterror = SQLSetEnvAttr(m_hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
	if(!SQL_SUCCEEDED(m_lasterror))
		return false;

	m_lasterror = SQLAllocHandle(SQL_HANDLE_DBC,m_hEnv,&m_hDbc);
	if(!SQL_SUCCEEDED(m_lasterror))
		return false;

	cvs::wstring strConn;
	cvs::swprintf(strConn,80,L"DRIVER={IBM DB2 ODBC DRIVER};SERVER=\"%s\";DBALIAS=%s;UID=%s;PWD=%s",
		(const wchar_t*)cvs::wide(pCI->hostname.c_str()),(const wchar_t*)cvs::wide(pCI->database.c_str()),(const wchar_t*)cvs::wide(pCI->username.c_str()),(const wchar_t*)cvs::wide(pCI->password.c_str()));
	m_lasterror = SQLDriverConnect(m_hDbc,NULL,(SQLWCHAR*)strConn.c_str(),(SQLSMALLINT)strConn.size(),NULL,0,NULL,SQL_DRIVER_NOPROMPT);
	if(!SQL_SUCCEEDED(m_lasterror))
		return false;

	m_lasterror = SQLSetConnectAttr(m_hDbc,SQL_ATTR_AUTOCOMMIT,(SQLPOINTER)SQL_AUTOCOMMIT_ON,0);
	if(!SQL_SUCCEEDED(m_lasterror))
		return false;

	return true;
}

bool CDb2Connection::Close()
{
	if(m_hEnv)
	{
		SQLDisconnect(m_hDbc);
		SQLFreeConnect(m_hDbc);
		SQLFreeEnv(m_hEnv);
	}
	m_hEnv = NULL;
	m_hDbc = NULL;
	m_lastrsError=L"";
	return true;
}

bool CDb2Connection::IsOpen()
{
	if(m_hEnv)
		return true;
	return false;
}

CSqlRecordsetPtr CDb2Connection::Execute(const char *string, ...)
{
	cvs::string str;
	va_list va;

	va_start(va,string);
	cvs::vsprintf(str,64,string,va);
	va_end(va);

	return _Execute(str.c_str());
}

unsigned CDb2Connection::ExecuteAndReturnIdentity(const char *string, ...)
{
	cvs::string str;
	va_list va;

	va_start(va,string);
	cvs::vsprintf(str,64,string,va);
	va_end(va);

	str = "begin atomic;" + str;
	str += " select identity_val_local(); end";

	CSqlRecordsetPtr rs = _Execute(str.c_str());

	if(Error() || rs->Closed() || rs->Eof())
		return 0;
	return *rs[0];
}

CSqlRecordsetPtr CDb2Connection::_Execute(const char *string)
{
	SQLRETURN ret;

	CDb2Recordset *rs = new CDb2Recordset();

	CServerIo::trace(3,"%s",string);

	HSTMT hStmt;

	if(!SQL_SUCCEEDED(m_lasterror=SQLAllocHandle(SQL_HANDLE_STMT,m_hDbc,&hStmt)))
	{
		SQLFreeStmt(hStmt,SQL_DROP);
		return rs;
	}

	for(std::map<int,CSqlVariant>::iterator i = m_bindVars.begin(); i!=m_bindVars.end(); ++i)
	{
		switch(i->second.type())
		{
		case CSqlVariant::vtNull:
			m_sqli[i->first]=SQL_NULL_DATA;
			ret = SQLBindParameter(hStmt,i->first+1,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_CHAR,0,0,NULL,0,&m_sqli[i->first]);
			break;
		case CSqlVariant::vtChar:
			m_sqli[i->first]=0;
			m_sqlv[i->first].c=i->second;
			ret = SQLBindParameter(hStmt,i->first+1,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_CHAR,1,0,&m_sqlv[i->first].c,1,&m_sqli[i->first]);
			break;
		case CSqlVariant::vtUChar:
			m_sqli[i->first]=0;
			m_sqlv[i->first].c=i->second;
			ret = SQLBindParameter(hStmt,i->first+1,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_CHAR,1,0,&m_sqlv[i->first].c,1,&m_sqli[i->first]);
			break;
		case CSqlVariant::vtShort:
			m_sqli[i->first]=0;
			m_sqlv[i->first].s=i->second;
			ret = SQLBindParameter(hStmt,i->first+1,SQL_PARAM_INPUT,SQL_C_SSHORT,SQL_INTEGER,0,0,&m_sqlv[i->first].s,0,&m_sqli[i->first]);
			break;
		case CSqlVariant::vtUShort:
			m_sqli[i->first]=0;
			m_sqlv[i->first].s=i->second;
			ret = SQLBindParameter(hStmt,i->first+1,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_INTEGER,0,0,&m_sqlv[i->first].s,0,&m_sqli[i->first]);
			break;
		case CSqlVariant::vtInt:
		case CSqlVariant::vtLong:
			m_sqli[i->first]=0;
			m_sqlv[i->first].l=i->second;
			ret = SQLBindParameter(hStmt,i->first+1,SQL_PARAM_INPUT,SQL_C_SLONG,SQL_INTEGER,0,0,&m_sqlv[i->first].l,0,&m_sqli[i->first]);
			break;
		case CSqlVariant::vtUInt:
		case CSqlVariant::vtULong:
			m_sqli[i->first]=0;
			m_sqlv[i->first].l=i->second;
			ret = SQLBindParameter(hStmt,i->first+1,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&m_sqlv[i->first].l,0,&m_sqli[i->first]);
			break;
		case CSqlVariant::vtLongLong:
			m_sqli[i->first]=0;
			m_sqlv[i->first].ll=i->second;
			ret = SQLBindParameter(hStmt,i->first+1,SQL_PARAM_INPUT,SQL_C_SBIGINT,SQL_BIGINT,0,0,&m_sqlv[i->first].ll,0,&m_sqli[i->first]);
			break;
		case CSqlVariant::vtULongLong:
			m_sqli[i->first]=0;
			m_sqlv[i->first].ll=i->second;
			ret = SQLBindParameter(hStmt,i->first+1,SQL_PARAM_INPUT,SQL_C_UBIGINT,SQL_BIGINT,0,0,&m_sqlv[i->first].ll,0,&m_sqli[i->first]);
			break;
		case CSqlVariant::vtString:
			m_sqli[i->first]=SQL_NTS;
			m_sqlv[i->first].ws=cvs::wide(i->second);
			ret = SQLBindParameter(hStmt,i->first+1,SQL_PARAM_INPUT,SQL_C_WCHAR,SQL_WVARCHAR,(SQLINTEGER)m_sqlv[i->first].ws.size()+1,0,(SQLPOINTER)m_sqlv[i->first].ws.c_str(),(SQLINTEGER)m_sqlv[i->first].ws.size()+1,&m_sqli[i->first]);
			break;
		case CSqlVariant::vtWString:
			m_sqli[i->first]=SQL_NTS;
			m_sqlv[i->first].ws=i->second;
			ret = SQLBindParameter(hStmt,i->first+1,SQL_PARAM_INPUT,SQL_C_WCHAR,SQL_WVARCHAR,(SQLINTEGER)m_sqlv[i->first].ws.size()+1,0,(SQLPOINTER)m_sqlv[i->first].ws.c_str(),(SQLINTEGER)m_sqlv[i->first].ws.size()+1,&m_sqli[i->first]);
			break;
		}
	}

	rs->Init(this,hStmt,string); // Ignore return... it's handled by the error routines

	m_bindVars.clear();

	return rs;
}

bool CDb2Connection::Error() const
{
	if(SQL_SUCCEEDED(m_lasterror))
		return false;
	return true;
}

const char *CDb2Connection::ErrorString()
{
	SQLWCHAR state[6];
	SQLINTEGER error;
	SQLSMALLINT size = 512,len;
	cvs::wstring err;
	err.resize((int)size);
	SQLWCHAR *pmsg = (SQLWCHAR*)err.data();

	if(m_lastrsError.size())
	{
		wcscpy((wchar_t *)pmsg,m_lastrsError.c_str());
		pmsg+=m_lastrsError.size();
		size-=(SQLSMALLINT)m_lastrsError.size();
		m_lastrsError=L"";
	}
	if(m_hDbc)
	{
		for(int i=1; SQL_SUCCEEDED(SQLGetDiagRec(SQL_HANDLE_DBC, m_hDbc, i, state, &error, pmsg, size, &len)); i++)
		{
			size-=len;
			pmsg+=len;
		}
	}
	if(m_hEnv)
	{
		for(int i=1; SQL_SUCCEEDED(SQLGetDiagRec(SQL_HANDLE_ENV, m_hEnv, i, state, &error, pmsg, size, &len)); i++)
		{
			size-=len;
			pmsg+=len;
		}
	}
	err.resize(512-size);
	m_lasterrorString=cvs::narrow(err.c_str());
	return m_lasterrorString.c_str();
}

bool CDb2Connection::BeginTrans()
{
	m_lasterror = SQLSetConnectAttr(m_hDbc,SQL_ATTR_AUTOCOMMIT,(SQLPOINTER)SQL_AUTOCOMMIT_OFF,0);

	if(!SQL_SUCCEEDED(m_lasterror))
		return false;

	return true;
}

bool CDb2Connection::CommitTrans()
{
	m_lasterror = SQLEndTran(SQL_HANDLE_DBC,m_hDbc,SQL_COMMIT);

	if(!SQL_SUCCEEDED(m_lasterror))
		return false;

	m_lasterror = SQLSetConnectAttr(m_hDbc,SQL_ATTR_AUTOCOMMIT,(SQLPOINTER)SQL_AUTOCOMMIT_ON,0);
	if(!SQL_SUCCEEDED(m_lasterror))
		return false;

	return true;
}

bool CDb2Connection::RollbackTrans()
{
	m_lasterror = SQLEndTran(SQL_HANDLE_DBC,m_hDbc,SQL_ROLLBACK);

	if(!SQL_SUCCEEDED(m_lasterror))
		return false;

	m_lasterror = SQLSetConnectAttr(m_hDbc,SQL_ATTR_AUTOCOMMIT,(SQLPOINTER)SQL_AUTOCOMMIT_ON,0);
	if(!SQL_SUCCEEDED(m_lasterror))
		return false;

	return true;
}

bool CDb2Connection::Bind(int variable, CSqlVariant value)
{
	m_bindVars[variable]=value;
	return true;
}

CSqlConnectionInformation* CDb2Connection::GetConnectionInformation()
{
	if(!m_ConnectionInformation) m_ConnectionInformation = new CDb2ConnectionInformation();
	return m_ConnectionInformation;
}

const char *CDb2Connection::parseTableName(const char *szName)
{
	CDb2ConnectionInformation *pCI = static_cast<CDb2ConnectionInformation*>(GetConnectionInformation());

	if(!szName)
		return NULL;

	if(!pCI->prefix.size())
		return szName;

	return cvs::cache_static_string((pCI->prefix+szName).c_str());
}
