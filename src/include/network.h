// ----------------------------------------------------------------------------
// Copyright (C) 2014...2019
//              David Freese, W1HKJ
//
// This file is part of fldigi
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#ifndef NETWORK_H_
#define NETWORK_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cmath>

#include "mbedtls/config.h"
#include "mbedtls/net.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

extern bool get_http(const std::string& url, std::string& reply, double timeout = 0.0);

extern char ca_crt_rsa[];
extern size_t ca_crt_rsa_size;

//----------------------------------------------------------------------
//#define DEBUG_LEVEL 1

#define MBEDTLS_EXIT_SUCCESS    0
#define MBEDTLS_EXIT_FAILURE    1
#define MBEDTLS_DEBUG_C
#define MBEDTLS_CHECK_PARAMS

#define H_FIELD_SIZE     512
#define H_READ_SIZE     2048

typedef struct
{
    char method[8];
    int  status;
    char content_type[H_FIELD_SIZE];
    long content_length;
    bool chunked;
    bool close;
    char location[H_FIELD_SIZE];
    char referrer[H_FIELD_SIZE];
    char cookie[H_FIELD_SIZE];
    char boundary[H_FIELD_SIZE];

} HTTP_HEADER;

typedef struct
{
    bool    verify;

    mbedtls_net_context         ssl_fd;
    mbedtls_entropy_context     entropy;
    mbedtls_ctr_drbg_context    ctr_drbg;
    mbedtls_ssl_context         ssl;
    mbedtls_ssl_config          conf;
    mbedtls_x509_crt            cacert;

} HTTP_SSL;

typedef struct {

    bool    https;
    char    host[256];
    char    port[8];
    char    path[H_FIELD_SIZE];

} HTTP_URL;

typedef struct
{
    HTTP_URL    url;

    HTTP_HEADER request;
    HTTP_HEADER response;
    HTTP_SSL    tls;

    long        length;
    char        r_buf[H_READ_SIZE];
    long        r_len;
    bool        header_end;
    char        *body;
    long        body_size;
    long        body_len;


} HTTP_INFO;

//----------------------------------------------------------------------

class Url {
	std::string _url;
	std::string _host;
	std::string _port;
	std::string _request;
	std::string _data;
	std::string _pers;

	std::string server_port;

	bool _https;

	int  _err;
	char err_string[1024];
	char buf[4096];

	mbedtls_net_context server_fd;
	uint32_t flags;

	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;
	mbedtls_x509_crt cacert;

	bool _debug;
	static int _rotate_log;

	double _timeout;

	std::ofstream debug_file;

	int http_get(std::string &response);
	int https_get(std::string &response);

public:
	Url() {
		init();
	};
	Url(std::string url) {
		init();
		_url = url;
		parse(url);
	}
	~Url() {
		if (debug_file) {
			debug_file.close();
		}
	};

	void init() {
		_https = false;
		server_port.clear();
		_url.clear();
		_host.clear();
		_port.clear();
		_request.clear();
		_data.clear();
		_pers = "fldigi";
		_timeout = 5.0;
		debug();
	}

	void parse(std::string url);

	std::string host() { return _host; }
	std::string port() { return _port; }
	std::string request() { return _request; }
	std::string data() { return _data; }
	std::string url() { return _url; }
	std::string strerr() { return err_string; };
	int error() { return _err; }

	bool https() { return _https; }
	std::string str_https() {
		if (_https) return "true";
		return "false";
	}

	int get(std::string response);
	int get(std::string url, std::string &response);

	void timeout(double t) { _timeout = t; }
	double timeout() { return _timeout; }

	void debug();
};


#endif // NETWORK_H_
