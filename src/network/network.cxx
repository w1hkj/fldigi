// ----------------------------------------------------------------------------
// network.cxx
//
// Copyright (C) 2008...2019
//		David Freese, W1HKJ
//
// This file is part of fldigi.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <cmath>
#include <string>
#include <sstream>
#include <errno.h>
#include <pthread.h>
#include <time.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>

#include "socket.h"
#include "timeops.h"
#include "gettext.h"
#include "debug.h"
#include "main.h"

#include "network.h"
#include "socket.h"

#include "fl_digi.h"

using namespace std;

//----------------------------------------------------------------------
//
//----------------------------------------------------------------------
static void my_debug( void *ctx, int level,
					  const char *file, int line,
					  const char *str )
{
	((void) level);

	fprintf( (FILE *) ctx, "%s:%04d: %s", file, line, str );
	fflush(  (FILE *) ctx  );
}

std::string stripped(std::string s)
{
	while (s[0] == ' ') s.erase(0,1);
	while (s[s.length()-1] == ' ') s.erase(s.length() - 1);
	return s;
}

void Url::parse(std::string url)
{
	_https = false;
	_host.clear();
	_port.clear();
	_request.clear();
	_url = url;

	if (_url.find("https://") == 0) {
		_https = true;
		_url.erase(0,8);
		_port = "443";
	} else if (url.find("http://") == 0)  {
		_url.erase(0,7);
		_port = "80";
	}
	size_t p = _url.find(":");
	if (p != std::string::npos) {
		_port = _url.substr(p+1);
		_url.erase(p);
	}
	p = _url.find("/");
	if (p != std::string::npos) {
		_host = _url.substr(0, p);
		_request = _url.substr(p);
	} else {
		_host = _url;
	}
	_host = stripped(_host);
	_url = stripped(url);
	_request = stripped(_request);
	if (debug_file) {
		debug_file << "parser:" << std::endl;
		debug_file << "url:  " << url << std::endl;
		debug_file << "host: " << _host << std::endl;
		debug_file << "port: " << _port << std::endl;
		debug_file << "req:  " << _request << std::endl;
	}
}

int Url::http_get(std::string &response)
{
	std::ostringstream REQUEST;
	size_t len, rcvd;
	bool ret = true;

	const char service[] = "http";

	REQUEST << "GET " << _request << " HTTP/1.1\r\n" <<
"User-Agent: fldigi " << FLDIGI_VERSION << "\r\n\
Host: " << _host << "\r\n\
Connection: close\r\n\r\n";

	len = REQUEST.str().length();

	if (debug_file) {
		debug_file << "Url::http_get(...)" << std::endl;
		debug_file << "Host:    " << _host << std::endl;
		debug_file << "Service: " << service << std::endl;
		debug_file << "Request: " << REQUEST.str() << std::endl;
	}
	try {
		Address addr(_host.c_str(), service);
		Socket s(addr);
//		Socket s(Address(_host.c_str(), service));
		s.connect();
		s.set_nonblocking();
		s.set_timeout(_timeout);

		if (s.send(REQUEST.str()) != len) {
			if (debug_file)
				debug_file << "send timed out: " << REQUEST.str() << std::endl;
			response = "Send timed out";
			ret = false;
			goto recv_exit;
		}
		if (debug_file)
			debug_file << "sent: " << len << " bytes." << std::endl;

		int wait = 1; // allow up to 5 seconds to receive response 
		while ((rcvd = s.recv(response)) == 0) {
			wait++;
			if (wait > 5) {
				if (debug_file)
					debug_file << "s.recv(...) failed after " << wait << " attempts" << std::endl;
				return false;
			}
			MilliSleep(100);
		}
		if (debug_file)
			debug_file << "s.recv(...) response required " << wait << " request" << (wait > 1 ? "s" : "") << std::endl;

	} catch (const SocketException& e) {
		response = e.what();
		if (response.empty()) response = "UNKNOWN ERROR";
		if (debug_file) {
			debug_file << "Caught socket exception: " << errno << ": " << response << std::endl;
		}
		ret = false;
	}

	if (debug_file) {
		debug_file << "s.recv(...) " << rcvd << " bytes." << std::endl;
		debug_file << "Response: " << response << std::endl;
	}
recv_exit:
	return ret;
}

int Url::https_get(std::string &response)
{
	int ret = 1, len;
	_err = MBEDTLS_EXIT_SUCCESS;
	std::ostringstream REQUEST;

	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;
	mbedtls_x509_crt cacert;

#if defined(MBEDTLS_DEBUG_C)
	mbedtls_debug_set_threshold( 1 );
#endif

	/*
	 * 0. Initialize the RNG and the session data
	 */
	mbedtls_net_init( &server_fd );
	mbedtls_ssl_init( &ssl );
	mbedtls_ssl_config_init( &conf );
	mbedtls_x509_crt_init( &cacert );
	mbedtls_ctr_drbg_init( &ctr_drbg );

	if (debug_file) {
		debug_file << "\n  . Seeding the random number generator...";
		debug_file.flush();
	}
	mbedtls_entropy_init( &entropy );
	if ( ( ret = mbedtls_ctr_drbg_seed(
					&ctr_drbg,
					mbedtls_entropy_func,
					&entropy,
					(const unsigned char *)_pers.c_str(),
					_pers.length() ) ) != 0 ) {
		snprintf(err_string, sizeof(err_string),
			" failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret );
		_err = ret;
		goto exit;
	}
	if (debug_file) {
		debug_file << " ok\n";
	}
	/*
	 * 0. Initialize certificates
	 */
	if (debug_file) {
		debug_file << "  . Loading the CA root certificate ...";
		debug_file.flush();
	}
	ca_crt_rsa[ca_crt_rsa_size - 1] = 0;
	ret = mbedtls_x509_crt_parse(&cacert, (uint8_t *)ca_crt_rsa, ca_crt_rsa_size);

	if( ret < 0 ) {
		snprintf(err_string, sizeof(err_string),
			" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret );
		_err = ret;
		goto exit;
	}

	if (debug_file) {
		debug_file << " ok (" << ret << " skipped)\n";
	}

	/*
	 * 1. Start the connection
	 */
	if (debug_file) {
		debug_file << "  . Connecting to tcp/"
				   << _host << ":" << _port << "...";
		debug_file.flush();
	}

	if( ( ret = mbedtls_net_connect(
					&server_fd,
					_host.c_str(),
					_port.c_str(),
					MBEDTLS_NET_PROTO_TCP ) ) != 0 ) {
		snprintf(err_string, sizeof(err_string),
			" failed\n  ! mbedtls_net_connect returned %d\n\n", ret );
		_err = ret;
		goto exit;
	}

	if (debug_file) {
		debug_file << " ok\n";
	}
	/*
	 * 2. Setup stuff
	 */

	if (debug_file) {
		debug_file << "  . Setting up the SSL/TLS structure...";
		debug_file.flush();
	}
	if( ( ret = mbedtls_ssl_config_defaults( &conf,
					MBEDTLS_SSL_IS_CLIENT,
					MBEDTLS_SSL_TRANSPORT_STREAM,
					MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 ) {
		snprintf(err_string, sizeof(err_string),
			" failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret );
		_err = ret;
		goto exit;
	}

	if (debug_file) {
		debug_file << " ok\n";
	}
	/*
	 * 3. More stuff
	 */

	/* OPTIONAL is not optimal for security,
	 * but makes interop easier in this simplified example */
	mbedtls_ssl_conf_authmode( &conf, MBEDTLS_SSL_VERIFY_OPTIONAL );
	mbedtls_ssl_conf_ca_chain( &conf, &cacert, NULL );
	mbedtls_ssl_conf_rng( &conf, mbedtls_ctr_drbg_random, &ctr_drbg );
	mbedtls_ssl_conf_dbg( &conf, my_debug, stdout );

	if( ( ret = mbedtls_ssl_setup( &ssl, &conf ) ) != 0 ) {
		snprintf(err_string, sizeof(err_string),
			" failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret );
		_err = ret;
		goto exit;
	}

	if( ( ret = mbedtls_ssl_set_hostname( &ssl, _host.c_str() ) ) != 0 ) {
		snprintf(err_string, sizeof(err_string),
			" failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret );
		_err = ret;
		goto exit;
	}

	mbedtls_ssl_set_bio( &ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL );

	/*
	 * 4. Handshake
	 */
	if (debug_file) {
		debug_file << "  . Performing the SSL/TLS handshake...";
		debug_file.flush();
	}
	while( ( ret = mbedtls_ssl_handshake( &ssl ) ) != 0 ) {
		if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE ) {
			snprintf(err_string, sizeof(err_string),
				" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret );
			_err = ret;
			goto exit;
		}
	}

	if (debug_file) {
		debug_file << " ok\n";
	}
	/*
	 * 5. Verify the server certificate
	 */
	if (debug_file) {
		debug_file << "  . Verifying peer X.509 certificate...";
	}
	/* In real life, we probably want to bail out when ret != 0 */
	if( ( flags = mbedtls_ssl_get_verify_result( &ssl ) ) != 0 ) {
		char vrfy_buf[512];
	if (debug_file) {
		debug_file << " failed\n";
	}
		mbedtls_x509_crt_verify_info( vrfy_buf, sizeof( vrfy_buf ), "  ! ", flags );
	if (debug_file) {
		debug_file << vrfy_buf << std::endl;
	}
	} else
	if (debug_file) {
		debug_file << " ok\n";
	}
	/*
	 * 6. Write the GET request
	 */

	if (debug_file) {
		debug_file << "  > Write to server:";
		debug_file.flush();
	}

	REQUEST << "GET " << _request << " HTTP/1.1\r\n";
	REQUEST << 
"User-Agent: fldigi-" <<  FLDIGI_VERSION << "\r\n" <<
"Host: " << _host << ":" << _port << "\r\n" <<
"Content-Type: application/json; charset=utf-8\r\n" <<
"Connection: Keep-Alive\r\n\r\n";
	len = REQUEST.str().length();
	while( ( ret = mbedtls_ssl_write(
						&ssl,
						(const unsigned char *)REQUEST.str().c_str(),
						len ) ) <= 0 ) {
		if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE ) {
			snprintf(err_string, sizeof(err_string),
				" failed\n  ! mbedtls_ssl_write returned %d\n\n", ret );
			_err = ret;
			goto exit;
		}
	}

	len = ret;
	if (debug_file) {
		debug_file << len << " bytes written\n\"" << REQUEST.str() << "\"";
		debug_file.flush();
	}
	/*
	 * 7. Read the HTTP response
	 */
	if (debug_file) {
		debug_file << "\n  < Read from server:";
		debug_file.flush();
	}

	do {
		len = sizeof( buf ) - 1;
		memset( buf, 0, sizeof( buf ) );
		ret = mbedtls_ssl_read( &ssl, (unsigned char *)buf, len );

		if( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE ) {
			continue;
		}

		if( ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY ) {
			break;
		}

		if( ret < 0 ) {
			if (debug_file) {
				debug_file << "failed\n  ! mbedtls_ssl_read returned " << ret << std::endl;
			}
			break;
		}

		if( ret == 0 ) {
			if (debug_file) {
				debug_file << "\n\nEOF\n\n";
			}
			break;
		}

		len = ret;
		if (debug_file) {
			debug_file << len << " bytes read\n\n" << (char *) buf << std::endl;
		}
		_data.append(buf);
		break;
	} while( 1 );

	mbedtls_ssl_close_notify( &ssl );

	response = _data;
	_err = MBEDTLS_EXIT_SUCCESS;

exit:

#ifdef MBEDTLS_ERROR_C
	if( _err != MBEDTLS_EXIT_SUCCESS ) {
		char error_buf[100];
		mbedtls_strerror( _err, error_buf, 100 );
		snprintf(err_string, sizeof(err_string),
			"Last error was: %d - %s\n\n", _err, error_buf );
	}
#endif

	mbedtls_net_free( &server_fd );

	mbedtls_x509_crt_free( &cacert );
	mbedtls_ssl_free( &ssl );
	mbedtls_ssl_config_free( &conf );
	mbedtls_ctr_drbg_free( &ctr_drbg );
	mbedtls_entropy_free( &entropy );

	return( _err );
}

int Url::get(std::string url, std::string &response)
{
	if (url.empty())
		return -1;

	parse(url);

	int ret = 0;
	if (_https)
		ret = https_get(response);
	else
		ret = http_get(response);

	return ret;
}

int Url::_rotate_log = 0;

void Url::debug()
{ 
	std::string fname = DebugDir;
	fname.append("network_debug.txt");
	if (!_rotate_log) {
		rotate_log(fname);
		_rotate_log = 1;
	}
	debug_file.open(fname.c_str(), ios::app);
}

bool get_http(const std::string& url, std::string& reply, double timeout)
{
	Url target_url;
	target_url.timeout(timeout);

	return target_url.get(url, reply);
}
