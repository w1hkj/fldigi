// ----------------------------------------------------------------------------
// Copyright (C) 2019
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

#ifndef HTTPS_CLIENT_HTTPS_H
#define HTTPS_CLIENT_HTTPS_H

/*
#include "mbedtls/net.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#define H_FIELD_SIZE     512
#define H_READ_SIZE     2048

//#undef TRUE
//#undef FALSE

//#define TRUE    1
//#define FALSE   0

//extern char ca_crt_rsa[];
//extern size_t ca_crt_rsa_size;

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
*/

#endif //HTTPS_CLIENT_HTTPS_H

