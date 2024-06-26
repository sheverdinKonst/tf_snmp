/**
 * \file debug.h
 *
 * \brief Debug functions
 *
 *  Copyright (C) 2006-2010, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef SSL_DEBUG_H
#define SSL_DEBUG_H

#include "config.h"
#include "ssl.h"

#if defined(POLARSSL_DEBUG_MSG) && defined(POLARSSL_DEBUG_C)

#define __FILENAME__ (strrchr(__FILE__, 0x5c) +1)

#define SSL_DEBUG_MSG( level, args )                    \
    debug_print_msg( ssl, level, __FILENAME__, __LINE__, debug_fmt args );

#define SSL_DEBUG_RET( level, text, ret )                \
    debug_print_ret( ssl, level, __FILENAME__, __LINE__, text, ret );

#define SSL_DEBUG_BUF( level, text, buf, len )           \
    debug_print_buf( ssl, level, __FILENAME__, __LINE__, text, buf, len );

#define SSL_DEBUG_MPI( level, text, X )                  \
    debug_print_mpi( ssl, level, __FILENAME__, __LINE__, text, X );

#define SSL_DEBUG_CRT( level, text, crt )                \
    debug_print_crt( ssl, level, __FILENAME__, __LINE__, text, crt );

#else

#define SSL_DEBUG_MSG( level, args )            do { } while( 0 )
#define SSL_DEBUG_RET( level, text, ret )       do { } while( 0 )
#define SSL_DEBUG_BUF( level, text, buf, len )  do { } while( 0 )
#define SSL_DEBUG_MPI( level, text, X )         do { } while( 0 )
#define SSL_DEBUG_CRT( level, text, crt )       do { } while( 0 )

#endif

#ifdef __cplusplus
extern "C" {
#endif

char *debug_fmt( const char *format, ... );

void debug_print_msg( const ssl_context *ssl, int level,
                      const char *file, int line, const char *text );

void debug_print_ret( const ssl_context *ssl, int level,
                      const char *file, int line,
                      const char *text, int ret );

void debug_print_buf( const ssl_context *ssl, int level,
                      const char *file, int line, const char *text,
                      unsigned char *buf, size_t len );

void debug_print_mpi( const ssl_context *ssl, int level,
                      const char *file, int line,
                      const char *text, const mpi *X );

void debug_print_crt( const ssl_context *ssl, int level,
                      const char *file, int line,
                      const char *text, const x509_cert *crt );

#ifdef __cplusplus
}
#endif

#endif /* debug.h */
