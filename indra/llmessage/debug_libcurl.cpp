#ifdef DEBUG_CURLIO

#include "sys.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <stdarg.h>
#include <cstring>
#include <algorithm>
#include <vector>
#include "llpreprocessor.h"
#include <curl/curl.h>
#define COMPILING_DEBUG_LIBCURL_CC
#include "debug_libcurl.h"
#include "debug.h"
#include "llerror.h"
#ifdef CWDEBUG
#include <libcwd/buf2str.h>
#endif

#define CURL_VERSION(major, minor, patch)	\
    (LIBCURL_VERSION_MAJOR > major ||		\
	 (LIBCURL_VERSION_MAJOR == major &&		\
	  LIBCURL_VERSION_MINOR > minor ||		\
	  (LIBCURL_VERSION_MINOR == minor &&	\
	   LIBCURL_VERSION_PATCH >= patch)))

static struct curl_slist unchanged_slist;

std::ostream& operator<<(std::ostream& os, struct curl_slist const& slist)
{
  struct curl_slist const* ptr = &slist;
  if (ptr == &unchanged_slist)
	os << " <unchanged> ";
  else
  {
	os << "(curl_slist)@0x" << std::hex << (size_t)ptr << std::dec << "{";
	do
	{
	  os << '"' << ptr->data << '"';
      ptr = ptr->next;
	  if (ptr)
		os << ", ";
	}
	while(ptr);
	os << "}";
  }
  return os;
}

#define CASEPRINT(x) case x: os << #x; break

std::ostream& operator<<(std::ostream& os, CURLINFO info)
{
  switch (info)
  {
	CASEPRINT(CURLINFO_EFFECTIVE_URL);
	CASEPRINT(CURLINFO_RESPONSE_CODE);
	CASEPRINT(CURLINFO_TOTAL_TIME);
	CASEPRINT(CURLINFO_NAMELOOKUP_TIME);
	CASEPRINT(CURLINFO_CONNECT_TIME);
	CASEPRINT(CURLINFO_PRETRANSFER_TIME);
	CASEPRINT(CURLINFO_SIZE_UPLOAD);
	CASEPRINT(CURLINFO_SIZE_DOWNLOAD);
	CASEPRINT(CURLINFO_SPEED_DOWNLOAD);
	CASEPRINT(CURLINFO_SPEED_UPLOAD);
	CASEPRINT(CURLINFO_HEADER_SIZE);
	CASEPRINT(CURLINFO_REQUEST_SIZE);
	CASEPRINT(CURLINFO_SSL_VERIFYRESULT);
	CASEPRINT(CURLINFO_FILETIME);
	CASEPRINT(CURLINFO_CONTENT_LENGTH_DOWNLOAD);
	CASEPRINT(CURLINFO_CONTENT_LENGTH_UPLOAD);
	CASEPRINT(CURLINFO_STARTTRANSFER_TIME);
	CASEPRINT(CURLINFO_CONTENT_TYPE);
	CASEPRINT(CURLINFO_REDIRECT_TIME);
	CASEPRINT(CURLINFO_REDIRECT_COUNT);
	CASEPRINT(CURLINFO_PRIVATE);
	CASEPRINT(CURLINFO_HTTP_CONNECTCODE);
	CASEPRINT(CURLINFO_HTTPAUTH_AVAIL);
	CASEPRINT(CURLINFO_PROXYAUTH_AVAIL);
	CASEPRINT(CURLINFO_OS_ERRNO);
	CASEPRINT(CURLINFO_NUM_CONNECTS);
	CASEPRINT(CURLINFO_SSL_ENGINES);
	CASEPRINT(CURLINFO_COOKIELIST);
	CASEPRINT(CURLINFO_LASTSOCKET);
	CASEPRINT(CURLINFO_FTP_ENTRY_PATH);
	CASEPRINT(CURLINFO_REDIRECT_URL);
	CASEPRINT(CURLINFO_PRIMARY_IP);
	CASEPRINT(CURLINFO_APPCONNECT_TIME);
	CASEPRINT(CURLINFO_CERTINFO);
	CASEPRINT(CURLINFO_CONDITION_UNMET);
	CASEPRINT(CURLINFO_RTSP_SESSION_ID);
	CASEPRINT(CURLINFO_RTSP_CLIENT_CSEQ);
	CASEPRINT(CURLINFO_RTSP_SERVER_CSEQ);
	CASEPRINT(CURLINFO_RTSP_CSEQ_RECV);
	CASEPRINT(CURLINFO_PRIMARY_PORT);
	CASEPRINT(CURLINFO_LOCAL_IP);
	CASEPRINT(CURLINFO_LOCAL_PORT);
	default:
      os << "<unknown" << " CURLINFO value (" << (int)info << ")>";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, CURLcode code)
{
  switch(code)
  {
	CASEPRINT(CURLE_OK);
	CASEPRINT(CURLE_UNSUPPORTED_PROTOCOL);
	CASEPRINT(CURLE_FAILED_INIT);
	CASEPRINT(CURLE_URL_MALFORMAT);
#if CURL_VERSION(7, 21, 5)
	CASEPRINT(CURLE_NOT_BUILT_IN);
#endif
	CASEPRINT(CURLE_COULDNT_RESOLVE_PROXY);
	CASEPRINT(CURLE_COULDNT_RESOLVE_HOST);
	CASEPRINT(CURLE_COULDNT_CONNECT);
	CASEPRINT(CURLE_FTP_WEIRD_SERVER_REPLY);
	CASEPRINT(CURLE_REMOTE_ACCESS_DENIED);
#if 0
	CASEPRINT(CURLE_FTP_ACCEPT_FAILED);
#endif
	CASEPRINT(CURLE_FTP_WEIRD_PASS_REPLY);
#if 0
	CASEPRINT(CURLE_FTP_ACCEPT_TIMEOUT);
#endif
	CASEPRINT(CURLE_FTP_WEIRD_PASV_REPLY);
	CASEPRINT(CURLE_FTP_WEIRD_227_FORMAT);
	CASEPRINT(CURLE_FTP_CANT_GET_HOST);
	CASEPRINT(CURLE_OBSOLETE16);
	CASEPRINT(CURLE_FTP_COULDNT_SET_TYPE);
	CASEPRINT(CURLE_PARTIAL_FILE);
	CASEPRINT(CURLE_FTP_COULDNT_RETR_FILE);
	CASEPRINT(CURLE_OBSOLETE20);
	CASEPRINT(CURLE_QUOTE_ERROR);
	CASEPRINT(CURLE_HTTP_RETURNED_ERROR);
	CASEPRINT(CURLE_WRITE_ERROR);
	CASEPRINT(CURLE_OBSOLETE24);
	CASEPRINT(CURLE_UPLOAD_FAILED);
	CASEPRINT(CURLE_READ_ERROR);
	CASEPRINT(CURLE_OUT_OF_MEMORY);
	CASEPRINT(CURLE_OPERATION_TIMEDOUT);
	CASEPRINT(CURLE_OBSOLETE29);
	CASEPRINT(CURLE_FTP_PORT_FAILED);
	CASEPRINT(CURLE_FTP_COULDNT_USE_REST);
	CASEPRINT(CURLE_OBSOLETE32);
	CASEPRINT(CURLE_RANGE_ERROR);
	CASEPRINT(CURLE_HTTP_POST_ERROR);
	CASEPRINT(CURLE_SSL_CONNECT_ERROR);
	CASEPRINT(CURLE_BAD_DOWNLOAD_RESUME);
	CASEPRINT(CURLE_FILE_COULDNT_READ_FILE);
	CASEPRINT(CURLE_LDAP_CANNOT_BIND);
	CASEPRINT(CURLE_LDAP_SEARCH_FAILED);
	CASEPRINT(CURLE_OBSOLETE40);
	CASEPRINT(CURLE_FUNCTION_NOT_FOUND);
	CASEPRINT(CURLE_ABORTED_BY_CALLBACK);
	CASEPRINT(CURLE_BAD_FUNCTION_ARGUMENT);
	CASEPRINT(CURLE_OBSOLETE44);
	CASEPRINT(CURLE_INTERFACE_FAILED);
	CASEPRINT(CURLE_OBSOLETE46);
	CASEPRINT(CURLE_TOO_MANY_REDIRECTS );
#if CURL_VERSION(7, 21, 5)
	CASEPRINT(CURLE_UNKNOWN_OPTION);
#else
	CASEPRINT(CURLE_UNKNOWN_TELNET_OPTION);
#endif
	CASEPRINT(CURLE_TELNET_OPTION_SYNTAX );
	CASEPRINT(CURLE_OBSOLETE50);
	CASEPRINT(CURLE_PEER_FAILED_VERIFICATION);
	CASEPRINT(CURLE_GOT_NOTHING);
	CASEPRINT(CURLE_SSL_ENGINE_NOTFOUND);
	CASEPRINT(CURLE_SSL_ENGINE_SETFAILED);
	CASEPRINT(CURLE_SEND_ERROR);
	CASEPRINT(CURLE_RECV_ERROR);
	CASEPRINT(CURLE_OBSOLETE57);
	CASEPRINT(CURLE_SSL_CERTPROBLEM);
	CASEPRINT(CURLE_SSL_CIPHER);
	CASEPRINT(CURLE_SSL_CACERT);
	CASEPRINT(CURLE_BAD_CONTENT_ENCODING);
	CASEPRINT(CURLE_LDAP_INVALID_URL);
	CASEPRINT(CURLE_FILESIZE_EXCEEDED);
	CASEPRINT(CURLE_USE_SSL_FAILED);
	CASEPRINT(CURLE_SEND_FAIL_REWIND);
	CASEPRINT(CURLE_SSL_ENGINE_INITFAILED);
	CASEPRINT(CURLE_LOGIN_DENIED);
	CASEPRINT(CURLE_TFTP_NOTFOUND);
	CASEPRINT(CURLE_TFTP_PERM);
	CASEPRINT(CURLE_REMOTE_DISK_FULL);
	CASEPRINT(CURLE_TFTP_ILLEGAL);
	CASEPRINT(CURLE_TFTP_UNKNOWNID);
	CASEPRINT(CURLE_REMOTE_FILE_EXISTS);
	CASEPRINT(CURLE_TFTP_NOSUCHUSER);
	CASEPRINT(CURLE_CONV_FAILED);
	CASEPRINT(CURLE_CONV_REQD);
	CASEPRINT(CURLE_SSL_CACERT_BADFILE);
	CASEPRINT(CURLE_REMOTE_FILE_NOT_FOUND);
	CASEPRINT(CURLE_SSH);
	CASEPRINT(CURLE_SSL_SHUTDOWN_FAILED);
	CASEPRINT(CURLE_AGAIN);
	CASEPRINT(CURLE_SSL_CRL_BADFILE);
	CASEPRINT(CURLE_SSL_ISSUER_ERROR);
	CASEPRINT(CURLE_FTP_PRET_FAILED);
	CASEPRINT(CURLE_RTSP_CSEQ_ERROR);
	CASEPRINT(CURLE_RTSP_SESSION_ERROR);
	CASEPRINT(CURLE_FTP_BAD_FILE_LIST);
	CASEPRINT(CURLE_CHUNK_FAILED);
    default:
      os << (code == CURL_LAST ? "<illegal" : "<unknown") << " CURLcode value (" << (int)code << ")>";
  }
  return os;
}

struct AICURL;
struct AICURLM;

std::ostream& operator<<(std::ostream& os, AICURL* curl)
{
  os << "(CURL*)0x" << std::hex << (size_t)curl << std::dec;
  return os;
}

std::ostream& operator<<(std::ostream& os, AICURLM* curl)
{
  os << "(CURLM*)0x" << std::hex << (size_t)curl << std::dec;
  return os;
}

std::ostream& operator<<(std::ostream& os, CURLoption option)
{
  switch(option)
  {
	CASEPRINT(CURLOPT_WRITEDATA);
	CASEPRINT(CURLOPT_URL);
	CASEPRINT(CURLOPT_PORT);
	CASEPRINT(CURLOPT_PROXY);
	CASEPRINT(CURLOPT_USERPWD);
	CASEPRINT(CURLOPT_PROXYUSERPWD);
	CASEPRINT(CURLOPT_RANGE);
	CASEPRINT(CURLOPT_READDATA);
	CASEPRINT(CURLOPT_ERRORBUFFER);
	CASEPRINT(CURLOPT_WRITEFUNCTION);
	CASEPRINT(CURLOPT_READFUNCTION);
	CASEPRINT(CURLOPT_TIMEOUT);
	CASEPRINT(CURLOPT_INFILESIZE);
	CASEPRINT(CURLOPT_POSTFIELDS);
	CASEPRINT(CURLOPT_REFERER);
	CASEPRINT(CURLOPT_FTPPORT);
	CASEPRINT(CURLOPT_USERAGENT);
	CASEPRINT(CURLOPT_LOW_SPEED_LIMIT);
	CASEPRINT(CURLOPT_LOW_SPEED_TIME);
	CASEPRINT(CURLOPT_RESUME_FROM);
	CASEPRINT(CURLOPT_COOKIE);
	CASEPRINT(CURLOPT_HTTPHEADER);
	CASEPRINT(CURLOPT_HTTPPOST);
	CASEPRINT(CURLOPT_SSLCERT);
	CASEPRINT(CURLOPT_KEYPASSWD);
	CASEPRINT(CURLOPT_CRLF);
	CASEPRINT(CURLOPT_QUOTE);
	CASEPRINT(CURLOPT_HEADERDATA);
	CASEPRINT(CURLOPT_COOKIEFILE);
	CASEPRINT(CURLOPT_SSLVERSION);
	CASEPRINT(CURLOPT_TIMECONDITION);
	CASEPRINT(CURLOPT_TIMEVALUE);
	CASEPRINT(CURLOPT_CUSTOMREQUEST);
	CASEPRINT(CURLOPT_STDERR);
	CASEPRINT(CURLOPT_POSTQUOTE);
	CASEPRINT(CURLOPT_WRITEINFO);
	CASEPRINT(CURLOPT_VERBOSE);
	CASEPRINT(CURLOPT_HEADER);
	CASEPRINT(CURLOPT_NOPROGRESS);
	CASEPRINT(CURLOPT_NOBODY);
	CASEPRINT(CURLOPT_FAILONERROR);
	CASEPRINT(CURLOPT_UPLOAD);
	CASEPRINT(CURLOPT_POST);
	CASEPRINT(CURLOPT_DIRLISTONLY);
	CASEPRINT(CURLOPT_APPEND);
	CASEPRINT(CURLOPT_NETRC);
	CASEPRINT(CURLOPT_FOLLOWLOCATION);
	CASEPRINT(CURLOPT_TRANSFERTEXT);
	CASEPRINT(CURLOPT_PUT);
	CASEPRINT(CURLOPT_PROGRESSFUNCTION);
	CASEPRINT(CURLOPT_PROGRESSDATA);
	CASEPRINT(CURLOPT_AUTOREFERER);
	CASEPRINT(CURLOPT_PROXYPORT);
	CASEPRINT(CURLOPT_POSTFIELDSIZE);
	CASEPRINT(CURLOPT_HTTPPROXYTUNNEL);
	CASEPRINT(CURLOPT_INTERFACE);
	CASEPRINT(CURLOPT_KRBLEVEL);
	CASEPRINT(CURLOPT_SSL_VERIFYPEER);
	CASEPRINT(CURLOPT_CAINFO);
	CASEPRINT(CURLOPT_MAXREDIRS);
	CASEPRINT(CURLOPT_FILETIME);
	CASEPRINT(CURLOPT_TELNETOPTIONS);
	CASEPRINT(CURLOPT_MAXCONNECTS);
	CASEPRINT(CURLOPT_CLOSEPOLICY);
	CASEPRINT(CURLOPT_FRESH_CONNECT);
	CASEPRINT(CURLOPT_FORBID_REUSE);
	CASEPRINT(CURLOPT_RANDOM_FILE);
	CASEPRINT(CURLOPT_EGDSOCKET);
	CASEPRINT(CURLOPT_CONNECTTIMEOUT);
	CASEPRINT(CURLOPT_HEADERFUNCTION);
	CASEPRINT(CURLOPT_HTTPGET);
	CASEPRINT(CURLOPT_SSL_VERIFYHOST);
	CASEPRINT(CURLOPT_COOKIEJAR);
	CASEPRINT(CURLOPT_SSL_CIPHER_LIST);
	CASEPRINT(CURLOPT_HTTP_VERSION);
	CASEPRINT(CURLOPT_FTP_USE_EPSV);
	CASEPRINT(CURLOPT_SSLCERTTYPE);
	CASEPRINT(CURLOPT_SSLKEY);
	CASEPRINT(CURLOPT_SSLKEYTYPE);
	CASEPRINT(CURLOPT_SSLENGINE);
	CASEPRINT(CURLOPT_SSLENGINE_DEFAULT);
	CASEPRINT(CURLOPT_DNS_USE_GLOBAL_CACHE);
	CASEPRINT(CURLOPT_DNS_CACHE_TIMEOUT);
	CASEPRINT(CURLOPT_PREQUOTE);
	CASEPRINT(CURLOPT_DEBUGFUNCTION);
	CASEPRINT(CURLOPT_DEBUGDATA);
	CASEPRINT(CURLOPT_COOKIESESSION);
	CASEPRINT(CURLOPT_CAPATH);
	CASEPRINT(CURLOPT_BUFFERSIZE);
	CASEPRINT(CURLOPT_NOSIGNAL);
	CASEPRINT(CURLOPT_SHARE);
	CASEPRINT(CURLOPT_PROXYTYPE);
#if CURL_VERSION(7, 21, 6)
	CASEPRINT(CURLOPT_ACCEPT_ENCODING);
#else
	CASEPRINT(CURLOPT_ENCODING);
#endif
	CASEPRINT(CURLOPT_PRIVATE);
	CASEPRINT(CURLOPT_HTTP200ALIASES);
	CASEPRINT(CURLOPT_UNRESTRICTED_AUTH);
	CASEPRINT(CURLOPT_FTP_USE_EPRT);
	CASEPRINT(CURLOPT_HTTPAUTH);
	CASEPRINT(CURLOPT_SSL_CTX_FUNCTION);
	CASEPRINT(CURLOPT_SSL_CTX_DATA);
	CASEPRINT(CURLOPT_FTP_CREATE_MISSING_DIRS);
	CASEPRINT(CURLOPT_PROXYAUTH);
	CASEPRINT(CURLOPT_FTP_RESPONSE_TIMEOUT);
	CASEPRINT(CURLOPT_IPRESOLVE);
	CASEPRINT(CURLOPT_MAXFILESIZE);
	CASEPRINT(CURLOPT_INFILESIZE_LARGE);
	CASEPRINT(CURLOPT_RESUME_FROM_LARGE);
	CASEPRINT(CURLOPT_MAXFILESIZE_LARGE);
	CASEPRINT(CURLOPT_NETRC_FILE);
	CASEPRINT(CURLOPT_USE_SSL);
	CASEPRINT(CURLOPT_POSTFIELDSIZE_LARGE);
	CASEPRINT(CURLOPT_TCP_NODELAY);
	CASEPRINT(CURLOPT_FTPSSLAUTH);
	CASEPRINT(CURLOPT_IOCTLFUNCTION);
	CASEPRINT(CURLOPT_IOCTLDATA);
	CASEPRINT(CURLOPT_FTP_ACCOUNT);
	CASEPRINT(CURLOPT_COOKIELIST);
	CASEPRINT(CURLOPT_IGNORE_CONTENT_LENGTH);
	CASEPRINT(CURLOPT_FTP_SKIP_PASV_IP);
	CASEPRINT(CURLOPT_FTP_FILEMETHOD);
	CASEPRINT(CURLOPT_LOCALPORT);
	CASEPRINT(CURLOPT_LOCALPORTRANGE);
	CASEPRINT(CURLOPT_CONNECT_ONLY);
	CASEPRINT(CURLOPT_CONV_FROM_NETWORK_FUNCTION);
	CASEPRINT(CURLOPT_CONV_TO_NETWORK_FUNCTION);
	CASEPRINT(CURLOPT_CONV_FROM_UTF8_FUNCTION);
	CASEPRINT(CURLOPT_MAX_SEND_SPEED_LARGE);
	CASEPRINT(CURLOPT_MAX_RECV_SPEED_LARGE);
	CASEPRINT(CURLOPT_FTP_ALTERNATIVE_TO_USER);
	CASEPRINT(CURLOPT_SOCKOPTFUNCTION);
	CASEPRINT(CURLOPT_SOCKOPTDATA);
	CASEPRINT(CURLOPT_SSL_SESSIONID_CACHE);
	CASEPRINT(CURLOPT_SSH_AUTH_TYPES);
	CASEPRINT(CURLOPT_SSH_PUBLIC_KEYFILE);
	CASEPRINT(CURLOPT_SSH_PRIVATE_KEYFILE);
	CASEPRINT(CURLOPT_FTP_SSL_CCC);
	CASEPRINT(CURLOPT_TIMEOUT_MS);
	CASEPRINT(CURLOPT_CONNECTTIMEOUT_MS);
	CASEPRINT(CURLOPT_HTTP_TRANSFER_DECODING);
	CASEPRINT(CURLOPT_HTTP_CONTENT_DECODING);
	CASEPRINT(CURLOPT_NEW_FILE_PERMS);
	CASEPRINT(CURLOPT_NEW_DIRECTORY_PERMS);
	CASEPRINT(CURLOPT_POSTREDIR);
	CASEPRINT(CURLOPT_SSH_HOST_PUBLIC_KEY_MD5);
	CASEPRINT(CURLOPT_OPENSOCKETFUNCTION);
	CASEPRINT(CURLOPT_OPENSOCKETDATA);
	CASEPRINT(CURLOPT_COPYPOSTFIELDS);
	CASEPRINT(CURLOPT_PROXY_TRANSFER_MODE);
	CASEPRINT(CURLOPT_SEEKFUNCTION);
	CASEPRINT(CURLOPT_SEEKDATA);
	CASEPRINT(CURLOPT_CRLFILE);
	CASEPRINT(CURLOPT_ISSUERCERT);
	CASEPRINT(CURLOPT_ADDRESS_SCOPE);
	CASEPRINT(CURLOPT_CERTINFO);
	CASEPRINT(CURLOPT_USERNAME);
	CASEPRINT(CURLOPT_PASSWORD);
	CASEPRINT(CURLOPT_PROXYUSERNAME);
	CASEPRINT(CURLOPT_PROXYPASSWORD);
	CASEPRINT(CURLOPT_NOPROXY);
	CASEPRINT(CURLOPT_TFTP_BLKSIZE);
	CASEPRINT(CURLOPT_SOCKS5_GSSAPI_SERVICE);
	CASEPRINT(CURLOPT_SOCKS5_GSSAPI_NEC);
	CASEPRINT(CURLOPT_PROTOCOLS);
	CASEPRINT(CURLOPT_REDIR_PROTOCOLS);
	CASEPRINT(CURLOPT_SSH_KNOWNHOSTS);
	CASEPRINT(CURLOPT_SSH_KEYFUNCTION);
	CASEPRINT(CURLOPT_SSH_KEYDATA);
	CASEPRINT(CURLOPT_MAIL_FROM);
	CASEPRINT(CURLOPT_MAIL_RCPT);
	CASEPRINT(CURLOPT_FTP_USE_PRET);
	CASEPRINT(CURLOPT_RTSP_REQUEST);
	CASEPRINT(CURLOPT_RTSP_SESSION_ID);
	CASEPRINT(CURLOPT_RTSP_STREAM_URI);
	CASEPRINT(CURLOPT_RTSP_TRANSPORT);
	CASEPRINT(CURLOPT_RTSP_CLIENT_CSEQ);
	CASEPRINT(CURLOPT_RTSP_SERVER_CSEQ);
	CASEPRINT(CURLOPT_INTERLEAVEDATA);
	CASEPRINT(CURLOPT_INTERLEAVEFUNCTION);
	CASEPRINT(CURLOPT_WILDCARDMATCH);
	CASEPRINT(CURLOPT_CHUNK_BGN_FUNCTION);
	CASEPRINT(CURLOPT_CHUNK_END_FUNCTION);
	CASEPRINT(CURLOPT_FNMATCH_FUNCTION);
	CASEPRINT(CURLOPT_CHUNK_DATA);
	CASEPRINT(CURLOPT_FNMATCH_DATA);
#if CURL_VERSION(7, 21, 3)
	CASEPRINT(CURLOPT_RESOLVE);
#endif
#if CURL_VERSION(7, 21, 4)
	CASEPRINT(CURLOPT_TLSAUTH_USERNAME);
	CASEPRINT(CURLOPT_TLSAUTH_PASSWORD);
	CASEPRINT(CURLOPT_TLSAUTH_TYPE);
#endif
#if CURL_VERSION(7, 21, 6)
	CASEPRINT(CURLOPT_TRANSFER_ENCODING);
#endif
#if CURL_VERSION(7, 21, 7)
	CASEPRINT(CURLOPT_CLOSESOCKETFUNCTION);
	CASEPRINT(CURLOPT_CLOSESOCKETDATA);
#endif
#if CURL_VERSION(7, 22, 0)
	CASEPRINT(CURLOPT_GSSAPI_DELEGATION);
#endif
#if CURL_VERSION(7, 24, 0)
	CASEPRINT(CURLOPT_DNS_SERVERS);
	CASEPRINT(CURLOPT_ACCEPTTIMEOUT_MS);
#endif
#if CURL_VERSION(7, 25, 0)
	CASEPRINT(CURLOPT_TCP_KEEPALIVE);
	CASEPRINT(CURLOPT_TCP_KEEPIDLE);
	CASEPRINT(CURLOPT_TCP_KEEPINTVL);
#endif
#if CURL_VERSION(7, 25, 0)
	CASEPRINT(CURLOPT_SSL_OPTIONS);
	CASEPRINT(CURLOPT_MAIL_AUTH);
#endif
	default:
      os << "<unknown CURLoption value (" << (int)option << ")>";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, CURLMoption option)
{
  switch(option)
  {
    CASEPRINT(CURLMOPT_SOCKETFUNCTION);
	CASEPRINT(CURLMOPT_SOCKETDATA);
	CASEPRINT(CURLMOPT_PIPELINING);
	CASEPRINT(CURLMOPT_TIMERFUNCTION);
	CASEPRINT(CURLMOPT_TIMERDATA);
	CASEPRINT(CURLMOPT_MAXCONNECTS);
	default:
      os << "<unknown CURLMoption value (" << (int)option << ")>";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, CURLMsg* msg)
{
  if (msg)
  {
	os << "(CURLMsg*){";
	if (msg->msg == CURLMSG_DONE)
	  os << "CURLMSG_DONE";
	else
	  os << msg->msg;
	os << ", " << (AICURL*)msg->easy_handle << ", 0x" << std::hex << (size_t)msg->data.whatever << std::dec << '}';
  }
  else
	os << "(CURLMsg*)NULL";
  return os;
}

struct Socket {
  curl_socket_t mSocket;
  Socket(curl_socket_t sockfd) : mSocket(sockfd) { }
};

std::ostream& operator<<(std::ostream& os, Socket const& sock)
{
  if (sock.mSocket == CURL_SOCKET_TIMEOUT)
	os << "CURL_SOCKET_TIMEOUT";
  else
	os << sock.mSocket;
  return os;
}

struct EvBitmask {
  int mBitmask;
  EvBitmask(int mask) : mBitmask(mask) { }
};

std::ostream& operator<<(std::ostream& os, EvBitmask const& bitmask)
{
  int m = bitmask.mBitmask;
  if (m == 0)
	os << '0';
  if ((m & CURL_CSELECT_IN))
  {
	os << "CURL_CSELECT_IN";
	if ((m & (CURL_CSELECT_OUT|CURL_CSELECT_ERR)))
	  os << '|';
  }
  if ((m & CURL_CSELECT_OUT))
  {
	os << "CURL_CSELECT_OUT";
	if ((m & CURL_CSELECT_ERR))
	  os << '|';
  }
  if ((m & CURL_CSELECT_ERR))
  {
	os << "CURL_CSELECT_ERR";
  }
  return os;
}

// Set this to limit the curl debug output to specific easy handles.
bool gDebugCurlTerse = false;

namespace {

std::vector<CURL*> handles;

inline bool print_debug(CURL* handle)
{
  if (!gDebugCurlTerse)
	return true;
  return std::find(handles.begin(), handles.end(), handle) != handles.end();
}

} // namespace

void debug_curl_add_easy(CURL* handle)
{
  std::vector<CURL*>::iterator iter = std::find(handles.begin(), handles.end(), handle);
  if (iter == handles.end())
  {
	handles.push_back(handle);
	Dout(dc::warning, "debug_curl_add_easy(" << (void*)handle << "): added");
  }
  llassert(print_debug(handle));
}

void debug_curl_remove_easy(CURL* handle)
{
  std::vector<CURL*>::iterator iter = std::find(handles.begin(), handles.end(), handle);
  if (iter != handles.end())
  {
	handles.erase(iter);
	Dout(dc::warning, "debug_curl_remove_easy(" << (void*)handle << "): removed");
  }
  llassert(!gDebugCurlTerse || !print_debug(handle));
}

bool debug_curl_print_debug(CURL* handle)
{
  return print_debug(handle);
}

extern "C" {

void debug_curl_easy_cleanup(CURL* handle)
{
  Dout(dc::curltr(print_debug(handle)), "curl_easy_cleanup(" << (AICURL*)handle << ")");
  curl_easy_cleanup(handle);
}

CURL* debug_curl_easy_duphandle(CURL* handle)
{
  Dout(dc::curltr(print_debug(handle))|continued_cf, "curl_easy_duphandle(" << (AICURL*)handle << ") = ");
  CURL* ret = curl_easy_duphandle(handle);
  Dout(dc::finish, (AICURL*)ret);
  return ret;
}

char* debug_curl_easy_escape(CURL* curl, char* url, int length)
{
  Dout(dc::curltr(print_debug(curl))|continued_cf, "curl_easy_escape(" << (AICURL*)curl << ", \"" << url << "\", " << length << ") = ");
  char* ret = curl_easy_escape(curl, url, length);
  Dout(dc::finish, '"' << ret << '"');
  return ret;
}

CURLcode debug_curl_easy_getinfo(CURL* handle, CURLINFO info, ...)
{
  va_list ap;
  union param_type {
	void* some_ptr;
	long* long_ptr;
	char** char_ptr;
	curl_slist** curl_slist_ptr;
	double* double_ptr;
  } param;
  va_start(ap, info);
  param.some_ptr = va_arg(ap, void*);
  va_end(ap);
  Dout(dc::curltr(print_debug(handle))|continued_cf, "curl_easy_getinfo(" << (AICURL*)handle << ", " << info << ", ");
  CURLcode ret = curl_easy_getinfo(handle, info, param.some_ptr);
  if (info == CURLINFO_PRIVATE)
  {
	Dout(dc::finish, "0x" << std::hex << (size_t)param.some_ptr << std::dec << ") = " << ret);
  }
  else
  {
	switch((info & CURLINFO_TYPEMASK))
	{
	  case CURLINFO_STRING:
		Dout(dc::finish, "(char**){ \"" << (ret == CURLE_OK ? *param.char_ptr : " <unchanged> ") << "\" }) = " << ret);
		break;
	  case CURLINFO_LONG:
		Dout(dc::finish, "(long*){ " << (ret == CURLE_OK ? *param.long_ptr : 0L) << "L }) = " << ret);
		break;
	  case CURLINFO_DOUBLE:
		Dout(dc::finish, "(double*){" << (ret == CURLE_OK ? *param.double_ptr : 0.) << "}) = " << ret);
		break;
	  case CURLINFO_SLIST:
		Dout(dc::finish, "(curl_slist**){ " << (ret == CURLE_OK ? **param.curl_slist_ptr : unchanged_slist) << " }) = " << ret);
		break;
	}
  }
  return ret;
}

CURL* debug_curl_easy_init(void)
{
  Dout(dc::curltr(!gDebugCurlTerse)|continued_cf, "curl_easy_init() = ");
  CURL* ret = curl_easy_init();
  Dout(dc::finish, (AICURL*)ret);
  return ret;
}

CURLcode debug_curl_easy_pause(CURL* handle, int bitmask)
{
  Dout(dc::curltr(print_debug(handle))|continued_cf, "curl_easy_pause(" << (AICURL*)handle << ", 0x" << std::hex << bitmask << std::dec << ") = ");
  CURLcode ret = curl_easy_pause(handle, bitmask);
  Dout(dc::finish, ret);
  return ret;
}

CURLcode debug_curl_easy_perform(CURL* handle)
{
  Dout(dc::curltr(print_debug(handle))|continued_cf, "curl_easy_perform(" << (AICURL*)handle << ") = ");
  CURLcode ret = curl_easy_perform(handle);
  Dout(dc::finish, ret);
  return ret;
}

void debug_curl_easy_reset(CURL* handle)
{
  Dout(dc::curltr(print_debug(handle)), "curl_easy_reset(" << (AICURL*)handle << ")");
  curl_easy_reset(handle);
}

CURLcode debug_curl_easy_setopt(CURL* handle, CURLoption option, ...)
{
  CURLcode ret = CURLE_OBSOLETE50;	// Suppress compiler warning.
  va_list ap;
  union param_type {
	long along;
	void* ptr;
	curl_off_t offset;
  } param;
  unsigned int param_type = (option / 10000) * 10000;
  va_start(ap, option);
  switch (param_type)
  {
	case CURLOPTTYPE_LONG:
	  param.along = va_arg(ap, long);
	  break;
	case CURLOPTTYPE_OBJECTPOINT:
	case CURLOPTTYPE_FUNCTIONPOINT:
	  param.ptr = va_arg(ap, void*);
	  break;
	case CURLOPTTYPE_OFF_T:
	  param.offset = va_arg(ap, curl_off_t);
	  break;
	default:
	  std::cerr << "Extracting param_type failed; option = " << option << "; param_type = " << param_type << std::endl;
	  std::exit(EXIT_FAILURE);
  }
  va_end(ap);
  static long postfieldsize;					// Cache. Assumes only one thread sets CURLOPT_POSTFIELDSIZE.
  switch (param_type)
  {
	case CURLOPTTYPE_LONG:
	{
	  Dout(dc::curltr(print_debug(handle))|continued_cf, "curl_easy_setopt(" << (AICURL*)handle << ", " << option << ", " << param.along << "L) = ");
	  ret = curl_easy_setopt(handle, option, param.along);
	  Dout(dc::finish, ret);
	  if (option == CURLOPT_POSTFIELDSIZE)
	  {
		postfieldsize = param.along;
	  }
	  break;
	}
	case CURLOPTTYPE_OBJECTPOINT:
	{
	  LibcwDoutScopeBegin(LIBCWD_DEBUGCHANNELS, libcwd::libcw_do, dc::curltr(print_debug(handle))|continued_cf)
	  LibcwDoutStream << "curl_easy_setopt(" << (AICURL*)handle << ", " << option << ", ";
	  // For a subset of all options that take a char*, print the string passed.
	  if (option == CURLOPT_PROXY ||			// Set HTTP proxy to use. The parameter should be a char* to a zero terminated string holding the host name or dotted IP address.
		  option == CURLOPT_PROXYUSERPWD ||		// Pass a char* as parameter, which should be [user name]:[password] to use for the connection to the HTTP proxy.
		  option == CURLOPT_CAINFO ||			// Pass a char * to a zero terminated string naming a file holding one or more certificates to verify the peer with.
		  option == CURLOPT_URL ||				// Pass in a pointer to the actual URL to deal with. The parameter should be a char * to a zero terminated string [...]
		  option == CURLOPT_COOKIEFILE ||		// Pass a pointer to a zero terminated string as parameter. It should contain the name of your file holding cookie data to read.
		  option == CURLOPT_CUSTOMREQUEST ||	// Pass a pointer to a zero terminated string as parameter. It can be used to specify the request instead of GET or HEAD when performing HTTP based requests
		  option == CURLOPT_ENCODING ||			// Sets the contents of the Accept-Encoding: header sent in a HTTP request, and enables decoding of a response when a Content-Encoding: header is received.
		  option == CURLOPT_POSTFIELDS ||
		  option == CURLOPT_COPYPOSTFIELDS)		// Full data to post in a HTTP POST operation.
	  {
		bool const is_postfield = option == CURLOPT_POSTFIELDS || option == CURLOPT_COPYPOSTFIELDS;
		char* str = (char*)param.ptr;
		long size;
		LibcwDoutStream << "(char*)0x" << std::hex << (size_t)param.ptr << std::dec << ")";
		if (is_postfield)
		{
		  size = postfieldsize < 32 ? postfieldsize : 32;	// Only print first 32 characters (this was already written to debug output before).
		}
		else
		{
		  size = strlen(str);
		}
		LibcwDoutStream << "[";
		if (str)
		{
		  LibcwDoutStream << libcwd::buf2str(str, size);
		  if (is_postfield && postfieldsize > 32)
			LibcwDoutStream << "...";
		}
		else
		{
		  LibcwDoutStream << "NULL";
		}
		LibcwDoutStream << "]";
		if (str)
		{
		  LibcwDoutStream << "(" << (is_postfield ? postfieldsize : size) << " bytes))";
		}
	  }
	  else
	  {
		LibcwDoutStream << "(object*)0x" << std::hex << (size_t)param.ptr << std::dec << ")";
	  }
	  LibcwDoutStream << " = ";
	  LibcwDoutScopeEnd;
	  ret = curl_easy_setopt(handle, option, param.ptr);
	  Dout(dc::finish, ret);
	  if (option == CURLOPT_HTTPHEADER && param.ptr)
	  {
		debug::Indent indent(2);
		Dout(dc::curltr, "HTTP Headers:");
		struct curl_slist* list = (struct curl_slist*)param.ptr;
		while (list)
		{
		  Dout(dc::curltr, '"' << list->data << '"');
		  list = list->next;
		}
	  }
	  break;
	}
	case CURLOPTTYPE_FUNCTIONPOINT:
	  Dout(dc::curltr(print_debug(handle))|continued_cf, "curl_easy_setopt(" << (AICURL*)handle << ", " << option << ", (function*)0x" << std::hex << (size_t)param.ptr << std::dec << ") = ");
	  ret = curl_easy_setopt(handle, option, param.ptr);
	  Dout(dc::finish, ret);
	  break;
	case CURLOPTTYPE_OFF_T:
	{
	  Dout(dc::curltr(print_debug(handle))|continued_cf, "curl_easy_setopt(" << (AICURL*)handle << ", " << option << ", (curl_off_t)" << param.offset << ") = ");
	  ret = curl_easy_setopt(handle, option, param.offset);
	  Dout(dc::finish, ret);
	  if (option == CURLOPT_POSTFIELDSIZE_LARGE)
	  {
		postfieldsize = (long)param.offset;
	  }
	  break;
    }
	default:
	  break;
  }
  return ret;
}

char const* debug_curl_easy_strerror(CURLcode errornum)
{
  Dout(dc::curltr(!gDebugCurlTerse)|continued_cf, "curl_easy_strerror(" << errornum << ") = ");
  char const* ret = curl_easy_strerror(errornum);
  Dout(dc::finish, '"' << ret << '"');
  return ret;
}

char* debug_curl_easy_unescape(CURL* curl, char* url, int inlength, int* outlength)
{
  Dout(dc::curltr(print_debug(curl))|continued_cf, "curl_easy_unescape(" << (AICURL*)curl << ", \"" << url << "\", " << inlength << ", ");
  char* ret = curl_easy_unescape(curl, url, inlength, outlength);
  Dout(dc::finish, ((ret && outlength) ? *outlength : 1) << ") = \"" << ret << '"');
  return ret;
}

void debug_curl_free(char* ptr)
{
  Dout(dc::curltr(!gDebugCurlTerse), "curl_free(0x" << std::hex << (size_t)ptr << std::dec << ")");
  curl_free(ptr);
}

time_t debug_curl_getdate(char const* datestring, time_t* now)
{
  Dout(dc::curltr(!gDebugCurlTerse)|continued_cf, "curl_getdate(\"" << datestring << "\", " << (now == NULL ? "NULL" : "<erroneous non-NULL value for 'now'>") << ") = ");
  time_t ret = curl_getdate(datestring, now);
  Dout(dc::finish, ret);
  return ret;
}

void debug_curl_global_cleanup(void)
{
  Dout(dc::curltr, "curl_global_cleanup()");
  curl_global_cleanup();
}

CURLcode debug_curl_global_init(long flags)
{
  Dout(dc::curltr|continued_cf, "curl_global_init(0x" << std::hex << flags << std::dec << ") = ");
  CURLcode ret = curl_global_init(flags);
  Dout(dc::finish, ret);
  return ret;
}

CURLMcode debug_curl_multi_add_handle(CURLM* multi_handle, CURL* easy_handle)
{
  Dout(dc::curltr(!gDebugCurlTerse)|continued_cf, "curl_multi_add_handle(" << (AICURLM*)multi_handle << ", " << (AICURL*)easy_handle << ") = ");
  CURLMcode ret = curl_multi_add_handle(multi_handle, easy_handle);
  Dout(dc::finish, ret);
  return ret;
}

CURLMcode debug_curl_multi_assign(CURLM* multi_handle, curl_socket_t sockfd, void* sockptr)
{
  Dout(dc::curltr(!gDebugCurlTerse)|continued_cf, "curl_multi_assign(" << (AICURLM*)multi_handle << ", " << Socket(sockfd) << ", " << sockptr << ") = ");
  CURLMcode ret = curl_multi_assign(multi_handle, sockfd, sockptr);
  Dout(dc::finish, ret);
  return ret;
}

CURLMcode debug_curl_multi_cleanup(CURLM* multi_handle)
{
  Dout(dc::curltr(!gDebugCurlTerse)|continued_cf, "curl_multi_cleanup(" << (AICURLM*)multi_handle << ") = ");
  CURLMcode ret = curl_multi_cleanup(multi_handle);
  Dout(dc::finish, ret);
  return ret;
}

CURLMsg* debug_curl_multi_info_read(CURLM* multi_handle, int* msgs_in_queue)
{
  Dout(dc::curltr(!gDebugCurlTerse)|continued_cf, "curl_multi_info_read(" << (AICURLM*)multi_handle << ", ");
  CURLMsg* ret = curl_multi_info_read(multi_handle, msgs_in_queue);
  Dout(dc::finish, "{" << *msgs_in_queue << "}) = " << ret);
  return ret;
}

CURLM* debug_curl_multi_init(void)
{
  Dout(dc::curltr(!gDebugCurlTerse)|continued_cf, "curl_multi_init() = ");
  CURLM* ret = curl_multi_init();
  Dout(dc::finish, (AICURLM*)ret);
  return ret;
}

CURLMcode debug_curl_multi_remove_handle(CURLM* multi_handle, CURL* easy_handle)
{
  Dout(dc::curltr(print_debug(easy_handle))|continued_cf, "curl_multi_remove_handle(" << (AICURLM*)multi_handle << ", " << (AICURL*)easy_handle << ") = ");
  CURLMcode ret = curl_multi_remove_handle(multi_handle, easy_handle);
  Dout(dc::finish, ret);
  return ret;
}

CURLMcode debug_curl_multi_setopt(CURLM* multi_handle, CURLMoption option, ...)
{
  CURLMcode ret = CURLM_UNKNOWN_OPTION;	// Suppress compiler warning.
  va_list ap;
  union param_type {
	long along;
	void* ptr;
	curl_off_t offset;
  } param;
  unsigned int param_type = (option / 10000) * 10000;
  va_start(ap, option);
  switch (param_type)
  {
	case CURLOPTTYPE_LONG:
	  param.along = va_arg(ap, long);
	  break;
	case CURLOPTTYPE_OBJECTPOINT:
	case CURLOPTTYPE_FUNCTIONPOINT:
	  param.ptr = va_arg(ap, void*);
	  break;
	case CURLOPTTYPE_OFF_T:
	  param.offset = va_arg(ap, curl_off_t);
	  break;
	default:
	  std::cerr << "Extracting param_type failed; option = " << option << "; param_type = " << param_type << std::endl;
	  std::exit(EXIT_FAILURE);
  }
  va_end(ap);
  switch (param_type)
  {
	case CURLOPTTYPE_LONG:
	  Dout(dc::curltr(!gDebugCurlTerse)|continued_cf, "curl_easy_setopt(" << (AICURLM*)multi_handle << ", " << option << ", " << param.along << "L) = ");
	  ret = curl_multi_setopt(multi_handle, option, param.along);
	  Dout(dc::finish, ret);
	  break;
	case CURLOPTTYPE_OBJECTPOINT:
	  Dout(dc::curltr(!gDebugCurlTerse)|continued_cf, "curl_easy_setopt(" << (AICURLM*)multi_handle << ", " << option << ", (object*)0x" << std::hex << (size_t)param.ptr << std::dec << ") = ");
	  ret = curl_multi_setopt(multi_handle, option, param.ptr);
	  Dout(dc::finish, ret);
	  break;
	case CURLOPTTYPE_FUNCTIONPOINT:
	  Dout(dc::curltr(!gDebugCurlTerse)|continued_cf, "curl_easy_setopt(" << (AICURLM*)multi_handle << ", " << option << ", (function*)0x" << std::hex << (size_t)param.ptr << std::dec << ") = ");
	  ret = curl_multi_setopt(multi_handle, option, param.ptr);
	  Dout(dc::finish, ret);
	  break;
	case CURLOPTTYPE_OFF_T:
	  Dout(dc::curltr(!gDebugCurlTerse)|continued_cf, "curl_easy_setopt(" << (AICURLM*)multi_handle << ", " << option << ", (curl_off_t)" << param.offset << ") = ");
	  ret = curl_multi_setopt(multi_handle, option, param.offset);
	  Dout(dc::finish, ret);
	  break;
	default:	// Stop compiler complaining about no default.
	  break;
  }
  return ret;
}

CURLMcode debug_curl_multi_socket_action(CURLM* multi_handle, curl_socket_t sockfd, int ev_bitmask, int* running_handles)
{
  Dout(dc::curltr(!gDebugCurlTerse)|continued_cf, "curl_multi_socket_action(" << (AICURLM*)multi_handle << ", " << Socket(sockfd) << ", " << EvBitmask(ev_bitmask) << ", ");
  CURLMcode ret = curl_multi_socket_action(multi_handle, sockfd, ev_bitmask, running_handles);
  Dout(dc::finish, "{" << (ret == CURLM_OK ? *running_handles : 0) << "}) = " << ret);
  return ret;
}

char const* debug_curl_multi_strerror(CURLMcode errornum)
{
  Dout(dc::curltr(!gDebugCurlTerse)|continued_cf, "curl_multi_strerror(" << errornum << ") = ");
  char const* ret = curl_multi_strerror(errornum);
  Dout(dc::finish, '"' << ret << '"');
  return ret;
}

struct curl_slist* debug_curl_slist_append(struct curl_slist* list, char const* string)
{
  Dout(dc::curltr(!gDebugCurlTerse)|continued_cf, "curl_slist_append((curl_slist)@0x" << std::hex << (size_t)list << std::dec << ", \"" << string << "\") = ");
  struct curl_slist* ret = curl_slist_append(list, string);
  Dout(dc::finish, *ret);
  return ret;
}

void debug_curl_slist_free_all(struct curl_slist* list)
{
  Dout(dc::curltr(!gDebugCurlTerse), "curl_slist_free_all((curl_slist)@0x" << std::hex << (size_t)list << std::dec << ")");
  curl_slist_free_all(list);
}

char* debug_curl_unescape(char const* url, int length)
{
  Dout(dc::curltr(!gDebugCurlTerse)|continued_cf, "curl_unescape(\"" << url << "\", " << length << ") = ");
  char* ret = curl_unescape(url, length);
  Dout(dc::finish, '"' << ret << '"');
  return ret;
}

char* debug_curl_version(void)
{
  Dout(dc::curltr(!gDebugCurlTerse)|continued_cf, "curl_version() = ");
  char* ret = curl_version();
  Dout(dc::finish, '"' << ret << '"');
  return ret;
}

}

#else	// DEBUG_CURLIO
int debug_libcurl_dummy;	// I thought some OS didn't like empty source files.
#endif	// DEBUG_CURLIO

