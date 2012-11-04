/* CVS sync auth interface
    Copyright (C) 2004-6 Tony Hoyle and March-Hare Software Ltd

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
#ifdef _WIN32
// Microsoft braindamage reversal.  
#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <io.h>
#else
#include <netdb.h>
#include <pwd.h>
#include <unistd.h>
#endif

/* Requires openssl installed */
#include <openssl/ssl.h>
#include <openssl/err.h>

#define MODULE sync

#include "common.h"
#include "../version.h"

#ifdef _WIN32
#include "sync_resource.h"
#endif

static int sync_connect(const struct protocol_interface *protocol, int verify_only);
static int sync_disconnect(const struct protocol_interface *protocol);
static int sync_auth_protocol_connect(const struct protocol_interface *protocol, const char *auth_string);
static int sync_read_data(const struct protocol_interface *protocol, void *data, int length);
static int sync_write_data(const struct protocol_interface *protocol, const void *data, int length);
static int sync_flush_data(const struct protocol_interface *protocol);
static int sync_shutdown(const struct protocol_interface *protocol);

static int sync_printf(char *fmt, ...);
static void sync_error(const char *txt, int err);

static SSL *ssl;
static SSL_CTX *ctx;
static int error_state;
static bool inauth;

#define SYNC_VERSION "0.1"
#define SYNC_INIT_STRING "SYNC "SYNC_VERSION" READY\n"
#define SYNC_CLIENT_VERSION_STRING "SYNC-CLIENT "SYNC_VERSION" READY\n"

int sync_win32config(const struct plugin_interface *ui, void *wnd);

static int init(const struct plugin_interface *plugin);
static int destroy(const struct plugin_interface *plugin);
static void *get_interface(const struct plugin_interface *plugin, unsigned interface_type, void *param);

struct protocol_interface sync_protocol_interface =
{
	{
		PLUGIN_INTERFACE_VERSION,
		"server sync protocol",CVSNT_PRODUCTVERSION_STRING,"SyncProtocol",
		init,
		destroy,
		get_interface,
	#ifdef _WIN32
		sync_win32config
	#else
		NULL
	#endif
	},
	"sync",
	"sync "CVSNT_PRODUCTVERSION_STRING,
	"(for server use only)",

	elemHostname|elemPassword|flagAlwaysEncrypted|flagSystem, /* Required elements */
	elemHostname|elemPassword|elemPort|elemTunnel|flagAlwaysEncrypted|flagSystem, /* Valid elements */

	NULL, /* validate */
	sync_connect,
	sync_disconnect,
	NULL, /* login */
	NULL, /* logout */
	NULL, /* wrap */
	sync_auth_protocol_connect,
	sync_read_data,
	sync_write_data,
	sync_flush_data,
	sync_shutdown,
	NULL, /* impersonate */
	NULL, /* validate_keyword */
	NULL, /* get_keyword_help */
	sync_read_data,
	sync_write_data,
	sync_flush_data,
	sync_shutdown,
};

static int init(const struct plugin_interface *plugin)
{
	error_state = 0;
	inauth = false;
	return 0;
}

static int destroy(const struct plugin_interface *plugin)
{
	protocol_interface *protocol = (protocol_interface*)plugin;
	free(protocol->auth_repository);
	free(protocol->auth_proxyname);
	free(protocol->auth_optional_3);
	if(ssl)
	{
	  SSL_free (ssl);
	  ssl=NULL;
	}
	if(ctx)
	{
      SSL_CTX_free (ctx);
	  ctx=NULL;
	}
	return 0;
}

static void *get_interface(const struct plugin_interface *plugin, unsigned interface_type, void *param)
{
	if(interface_type!=pitProtocol)
		return NULL;

	set_current_server((const struct server_interface*)param);
	return (void*)&sync_protocol_interface;
}

plugin_interface *get_plugin_interface()
{
	return &sync_protocol_interface.plugin;
}

int sync_connect(const struct protocol_interface *protocol, int verify_only)
{
	char server_version[128];
	const char *begin_request = "BEGIN SERVER SYNC REQUEST";
	const char *end_request = "END SERVER SYNC REQUEST";
	const char *username = NULL;
	const char *key = NULL;
	int err,l;
	char certs[4096];

	if(verify_only!=2)
		return CVSPROTO_NOTIMP;

	snprintf(certs,sizeof(certs),"%s/ca.pem",PATCH_NULL(current_server()->config_dir));

	if(!current_server()->current_root->hostname || !current_server()->current_root->password ||
		!current_server()->current_root->optional_1 || !current_server()->current_root->optional_2 ||
		!current_server()->current_root->optional_3)
		return CVSPROTO_BADPARMS;

	if(tcp_connect(current_server()->current_root))
		return CVSPROTO_FAIL;

	if(tcp_printf("%s\n",begin_request)<0)
		return CVSPROTO_FAIL;
	for(;;)
	{
		*server_version='\0';
		if((l=tcp_readline(server_version,sizeof(server_version))<0))
			return CVSPROTO_FAIL;
		if(*server_version)
			break;
#ifdef _WIN32
		Sleep(10);
#else
		usleep(10);
#endif
	  if(strncmp(server_version,"SYNC ",8))
	  {
	  	  server_error(0,"%s\n",server_version);
		  return CVSPROTO_FAIL;
	  }
	}

	SSL_library_init();
	SSL_load_error_strings ();
	ctx = SSL_CTX_new (SSLv3_client_method ());
	SSL_CTX_set_options(ctx,SSL_OP_ALL|SSL_OP_NO_SSLv2);
	SSL_CTX_load_verify_locations(ctx,certs,NULL);

	if(key)
	{
		if((err=SSL_CTX_use_certificate_file(ctx,key,SSL_FILETYPE_PEM))<1)
		{
			sync_error("Unable to read/load the client certificate", err);
			return CVSPROTO_FAIL;
		}

		if((err=SSL_CTX_use_PrivateKey_file(ctx,key,SSL_FILETYPE_PEM))<1)
		{
			sync_error("Unable to read/load the client private key", err);
			return CVSPROTO_FAIL;
		}

		if(!SSL_CTX_check_private_key(ctx))
		{
			sync_error("Client certificate failed verification", err);
			return CVSPROTO_FAIL;
		}
	}

//	if(strict)
//		SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT,NULL); /* Check verify result below */
//	else
		SSL_CTX_set_verify(ctx,SSL_VERIFY_NONE,NULL); /* Check verify result below */

    ssl = SSL_new (ctx);
	SSL_set_fd (ssl, get_tcp_fd());
    if((err=SSL_connect (ssl))<1)
	{
		/* if the SSL connection failed, it is probably because the server hasn't got a valid cert/key so is unable to setup a SSL session */
		sync_error("SSL connection failed", err);
		return CVSPROTO_FAIL;
	}

	switch(err=SSL_get_verify_result(ssl))
	{
		case X509_V_OK:
		case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
		/* Valid certificate */
		break;
		default:
			server_error(1,"Server certificate verification failed: %s\n",X509_verify_cert_error_string(err));
	}

	{
		X509 *cert;
		char buf[1024];

		if(!(cert=SSL_get_peer_certificate(ssl)))
			server_error(1,"Server did not present a valid certificate\n");

		buf[0]='\0';
//		if(strict)
//		{
//		  X509_NAME_get_text_by_NID(X509_get_subject_name(cert), NID_commonName, buf, sizeof(buf));
//		  if(strcasecmp(buf,current_server()->current_root->hostname))
//			server_error(1, "Certificate CommonName '%s' does not match server name '%s'\n",buf,current_server()->current_root->hostname);
//		}
	}

	if(sync_printf(SYNC_CLIENT_VERSION_STRING)<0)
		return CVSPROTO_FAIL;
	if(sync_printf("%s\n",current_server()->current_root->remote_repository)<0) // Remote repository name
		return CVSPROTO_FAIL;
	if(sync_printf("%s\n",current_server()->current_root->password)<0) // Passphrase
		return CVSPROTO_FAIL;
	if(sync_printf("%s\n",current_server()->current_root->optional_1)<0) // Local repository name
		return CVSPROTO_FAIL;
	if(sync_printf("%s\n",current_server()->current_root->optional_2)<0) // Local username
		return CVSPROTO_FAIL;
	if(sync_printf("%s\n",current_server()->current_root->optional_3)<0) // Sync type (proxy)
		return CVSPROTO_FAIL;
	if(sync_printf("%s\n",end_request)<0)
		return CVSPROTO_FAIL;
	return CVSPROTO_SUCCESS;
}

int sync_disconnect(const struct protocol_interface *protocol)
{
	if(tcp_disconnect())
		return CVSPROTO_FAIL;
	return CVSPROTO_SUCCESS;
}

int sync_auth_protocol_connect(const struct protocol_interface *protocol, const char *auth_string)
{
	char *tmp;
	int err;
	char certfile[1024];
	char keyfile[1024];
	char certs[4096];
	int certonly = 0;
	char *client_version = NULL;

    if (strcmp (auth_string, "BEGIN SERVER SYNC REQUEST"))
		return CVSPROTO_NOTME;

	inauth = true;

	sync_protocol_interface.verify_only = 0;

	write(current_server()->out_fd,SYNC_INIT_STRING,sizeof(SYNC_INIT_STRING)-1);

	if(CGlobalSettings::GetGlobalValue("cvsnt","PServer","CertificateFile",certfile,sizeof(certfile)))
	{
		server_error(0,"E Configuration Error - CertificateFile not specified\n");
		return CVSPROTO_FAIL;
	}
	if(CGlobalSettings::GetGlobalValue("cvsnt","PServer","PrivateKeyFile",keyfile,sizeof(keyfile)))
		strncpy(keyfile,certfile,sizeof(keyfile));
	if(CGlobalSettings::GetGlobalValue("cvsnt","PServer","CAFile",certs,sizeof(certs)))
		snprintf(certs,sizeof(certs),"%s/ca.pem",PATCH_NULL(current_server()->config_dir));

	SSL_library_init();
	SSL_load_error_strings ();
	ctx = SSL_CTX_new (SSLv23_server_method ());
	SSL_CTX_set_options(ctx,SSL_OP_ALL|SSL_OP_NO_SSLv2);

	SSL_CTX_load_verify_locations(ctx,certs,NULL);

	ERR_get_error(); // Clear error stack
	if((err=SSL_CTX_use_certificate_file(ctx, certfile ,SSL_FILETYPE_PEM))<1)
	{
		sync_error("Unable to read/load the server certificate", err);
		return CVSPROTO_FAIL;
	}
	if((err=SSL_CTX_use_PrivateKey_file(ctx, keyfile ,SSL_FILETYPE_PEM))<1)
	{
		sync_error("Unable to read/load the server private key", err);
		return CVSPROTO_FAIL;
	}
	if(!SSL_CTX_check_private_key(ctx))
	{
		sync_error("Server certificate failed verification", err);
		return CVSPROTO_FAIL;
	}
	SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER,NULL); /* Check verify result below */

    ssl = SSL_new (ctx);
#ifdef _WIN32 /* Win32 is stupid... */
    SSL_set_rfd (ssl, _get_osfhandle(current_server()->in_fd));
    SSL_set_wfd (ssl, _get_osfhandle(current_server()->out_fd));
#else
    SSL_set_rfd (ssl, current_server()->in_fd);
    SSL_set_wfd (ssl, current_server()->out_fd);
#endif
    set_encrypted_channel(1); /* Error must go through us now */

	if((err=SSL_accept(ssl))<1)
	{
		sync_error("SSL connection failed", err);
		return CVSPROTO_FAIL;
	}

	switch(err=SSL_get_verify_result(ssl))
	{
		case X509_V_OK:
		/* Valid (or no) certificate */
		break;
		case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
		/* Self signed certificate */
		server_error(0,"E Client sent self-signed certificate.\n");
		return CVSPROTO_FAIL;
		default:
			server_error(0,"E Server certificate verification failed: %s\n",X509_verify_cert_error_string(err));
			return CVSPROTO_FAIL;
	}

	X509 *cert = SSL_get_peer_certificate(ssl);
	//if(!cert)
	// no client certificate

    /* Get the three important pieces of information in order. */
    /* See above comment about error handling.  */

	server_getline (protocol, &client_version, MAX_PATH);
	server_getline (protocol, &sync_protocol_interface.auth_repository, MAX_PATH);
   	server_getline (protocol, &tmp, MAX_PATH);

	// we abuse certs here basically because it's there.
	if(CGlobalSettings::GetGlobalValue("cvsnt","PServer","ServerPassphrase",certs,sizeof(certs)))
	{
   		CServerIo::trace(3,"No server passphrase set");
		server_error(0,"E Bad passphrase sent for sync connection request");
		return CVSPROTO_AUTHFAIL;
	}

	CServerIo::trace(4,"Known plaintext password is %s",certs);
	CServerIo::trace(4,"Sent encrypted password is %s",tmp);
	if(CCrypt::compare(certs,tmp))
	{
		free(tmp);
   		CServerIo::trace(3,"Server passphrase incorrect");
		server_error(0,"E Bad passphrase sent for sync connection request");
		return CVSPROTO_AUTHFAIL;
	}
	free(tmp);

   	server_getline (protocol, &sync_protocol_interface.auth_proxyname, MAX_PATH); // Client repository name
   	server_getline (protocol, &sync_protocol_interface.auth_username, MAX_PATH); // Client username
   	server_getline (protocol, &sync_protocol_interface.auth_optional_3, MAX_PATH); // Sync type

	if(client_version) free(client_version);
	client_version = NULL;

    /* ... and make sure the protocol ends on the right foot. */
    /* See above comment about error handling.  */
    server_getline(protocol, &tmp, MAX_PATH);
    if (strcmp (tmp,"END SERVER SYNC REQUEST"))
    {
		server_printf ("bad auth protocol end: %s\n", tmp);
		free(tmp);
		return CVSPROTO_FAIL;
    }

	free(tmp);

//	if(!cert)
//	{
//		server_error(0,"E Login requires a valid client certificate.\n");
//		return CVSPROTO_AUTHFAIL;
//	}

	inauth = false;

	return CVSPROTO_SUCCESS;
}

int sync_read_data(const struct protocol_interface *protocol, void *data, int length)
{
	int r,e;
	//if(error_state)
	//	return -1;

	r=SSL_read(ssl,data,length);
	switch(e=SSL_get_error(ssl,r))
	{
		case SSL_ERROR_NONE:
			return r;
		case SSL_ERROR_ZERO_RETURN:
			return 0;
		case SSL_ERROR_SYSCALL:
		default:
			error_state = 1;
			sync_error("Read data failed", e);
			return -1;
	}
}

int sync_write_data(const struct protocol_interface *protocol, const void *data, int length)
{
	int offset=0,r,e;

	if(!ssl)
		return length;  // Error handling - stop recursion

	//if(error_state)
	//	return -1;
	while(length)
	{
		r=SSL_write(ssl,((const char *)data)+offset,length);
		switch(e=SSL_get_error(ssl,r))
		{
		case SSL_ERROR_NONE:
			length -= r;
			offset += r;
			break;
		case SSL_ERROR_WANT_WRITE:
			break;
		default:
			error_state = 1;
			sync_error("Write data failed", e);
			return -1;
		}
	}
	return offset;
}

int sync_flush_data(const struct protocol_interface *protocol)
{
	return 0; // TCP/IP is always flushed
}

int sync_shutdown(const struct protocol_interface *protocol)
{
	SSL_shutdown(ssl);
	return 0;
}

int sync_printf(char *fmt, ...)
{
	char temp[1024];
	va_list va;

	va_start(va,fmt);

	vsnprintf(temp,sizeof(temp),fmt,va);

	va_end(va);

	return sync_write_data(NULL,temp,strlen(temp));
}

void sync_error(const char *txt, int err)
{
	char errbuf[1024];
	int e = ERR_get_error();
	if(e)
		ERR_error_string_n(e, errbuf, sizeof(errbuf));
	else
		strcpy(errbuf,"Server dropped connection.");
	if(inauth)
		server_error(0, "E %s (%d): %s\n",txt,err,errbuf);
	else
		server_error(0, "%s (%d): %s\n",txt,err,errbuf);
}

#ifdef _WIN32
static BOOL CALLBACK ConfigDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int nSel;
	char value[256];
	char certfile[MAX_PATH];
	CScramble scram;

	switch(uMsg)
	{
	case WM_INITDIALOG:
		nSel = 1;
		if(!CGlobalSettings::GetGlobalValue("cvsnt","Plugins","SyncProtocol",value,sizeof(value)))
			nSel = atoi(value);
		SendDlgItemMessage(hWnd,IDC_CHECK1,BM_SETCHECK,nSel,0);
		if(!nSel)
		{
			EnableWindow(GetDlgItem(hWnd,IDC_EDIT1),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_EDIT2),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_EDIT3),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_SSLCERT),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_PRIVATEKEY),FALSE);
		}
		if(!CGlobalSettings::GetGlobalValue("cvsnt","PServer","ServerPassphrase",value,sizeof(value)))
			SetDlgItemText(hWnd,IDC_EDIT1,value);
		if(!CGlobalSettings::GetGlobalValue("cvsnt","PServer","CertificateFile",certfile,sizeof(certfile)))
			SetDlgItemText(hWnd,IDC_EDIT2,certfile);
		if(!CGlobalSettings::GetGlobalValue("cvsnt","PServer","PrivateKeyFile",certfile,sizeof(certfile)))
			SetDlgItemText(hWnd,IDC_EDIT3,certfile);
		return FALSE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_CHECK1:
			nSel=SendDlgItemMessage(hWnd,IDC_CHECK1,BM_GETCHECK,0,0);
			EnableWindow(GetDlgItem(hWnd,IDC_EDIT1),nSel?TRUE:FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_EDIT2),nSel?TRUE:FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_EDIT3),nSel?TRUE:FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_SSLCERT),nSel?TRUE:FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_PRIVATEKEY),nSel?TRUE:FALSE);
			return TRUE;
		case IDC_SSLCERT:
			{
				OPENFILENAME ofn = { sizeof(OPENFILENAME), hWnd, NULL, "Certificate files\0*.pem\0", NULL, 0, 0, certfile, sizeof(certfile), NULL, 0, NULL, NULL, OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_HIDEREADONLY, 0, 0, "pem" };

				GetDlgItemText(hWnd,IDC_EDIT2,certfile,sizeof(certfile));
				if(GetOpenFileName(&ofn))
					SetDlgItemText(hWnd,IDC_EDIT2,certfile);
			}
			return TRUE;
		case IDC_PRIVATEKEY:
			{
				OPENFILENAME ofn = { sizeof(OPENFILENAME), hWnd, NULL, "Private key files\0*.pem\0", NULL, 0, 0, certfile, sizeof(certfile), NULL, 0, NULL, NULL, OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_HIDEREADONLY, 0, 0, "pem" };

				GetDlgItemText(hWnd,IDC_EDIT3,certfile,sizeof(certfile));
				if(GetOpenFileName(&ofn))
					SetDlgItemText(hWnd,IDC_EDIT3,certfile);
			}
			return TRUE;
		case IDOK:
			GetDlgItemText(hWnd,IDC_EDIT1,certfile,sizeof(certfile));
			if(strlen(certfile)<8)
			{
				MessageBox(hWnd,_T("Passphrase too short.  Must be at least 8 characters."),"Sync settings",MB_ICONSTOP|MB_OK);
				return TRUE;
			}
			CGlobalSettings::SetGlobalValue("cvsnt","PServer","ServerPassphrase",certfile);
			nSel=SendDlgItemMessage(hWnd,IDC_CHECK1,BM_GETCHECK,0,0);
			snprintf(value,sizeof(value),"%d",nSel);
            CGlobalSettings::SetGlobalValue("cvsnt","Plugins","SyncProtocol",value);
			GetDlgItemText(hWnd,IDC_EDIT2,certfile,sizeof(certfile));
			CGlobalSettings::SetGlobalValue("cvsnt","PServer","CertificateFile",certfile);
			GetDlgItemText(hWnd,IDC_EDIT3,certfile,sizeof(certfile));
			CGlobalSettings::SetGlobalValue("cvsnt","PServer","PrivateKeyFile",certfile);
		case IDCANCEL:
			EndDialog(hWnd,LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}

int sync_win32config(const struct plugin_interface *ui, void *wnd)
{
	HWND hWnd = (HWND)wnd;
	int ret = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, ConfigDlgProc);
	return ret==IDOK?0:-1;
}
#endif

