#ifndef USER_SETTINGS_H
#define USER_SETTINGS_H

#define vsnprintf _vsnprintf

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501

#ifdef __cplusplus
extern "C" {
#endif

#define NO_WOLFSSL_STUB

#undef  FP_MAX_BITS
#define FP_MAX_BITS 16384

#undef  TFM_TIMING_RESISTANT
#define TFM_TIMING_RESISTANT

#undef  WOLFSSL_STATIC_DH
#define WOLFSSL_STATIC_DH

#undef  WOLFSSL_STATIC_RSA
#define WOLFSSL_STATIC_RSA

#undef  OPENSSL_EXTRA
#define OPENSSL_EXTRA

/*
The commented out defines below are the equivalent of --enable-tls13.
Uncomment them to build wolfSSL with TLS 1.3 support as of v3.11.1-tls13-beta.
This is for experimenting only, afaict TLS 1.3 support doesn't appear to be
functioning correctly yet. https://github.com/wolfSSL/wolfssl/pull/943

#undef  WC_RSA_PSS
#define WC_RSA_PSS

#undef  WOLFSSL_TLS13
#define WOLFSSL_TLS13

#undef  HAVE_TLS_EXTENSIONS
#define HAVE_TLS_EXTENSIONS

#undef  HAVE_FFDHE_2048
#define HAVE_FFDHE_2048

#undef  HAVE_HKDF
#define HAVE_HKDF
*/

#undef  TFM_TIMING_RESISTANT
#define TFM_TIMING_RESISTANT

#undef  ECC_TIMING_RESISTANT
#define ECC_TIMING_RESISTANT

#undef  WC_RSA_BLINDING
#define WC_RSA_BLINDING

#undef  HAVE_AESGCM
#define HAVE_AESGCM

#undef  WOLFSSL_RIPEMD
#define WOLFSSL_RIPEMD

#undef  WOLFSSL_SHA512
#define WOLFSSL_SHA512

#undef  WOLFSSL_SHA384
#define WOLFSSL_SHA384

#undef  SESSION_CERTS
#define SESSION_CERTS

#undef  WOLFSSL_CERT_GEN
#define WOLFSSL_CERT_GEN

#undef  HAVE_ECC
#define HAVE_ECC

#undef  TFM_ECC256
#define TFM_ECC256

#undef  ECC_SHAMIR
#define ECC_SHAMIR

#undef  WOLFSSL_ALLOW_SSLV3
#define WOLFSSL_ALLOW_SSLV3

#undef  NO_RC4
#define NO_RC4

#undef  NO_HC128
#define NO_HC128

#undef  NO_RABBIT
#define NO_RABBIT

#undef  HAVE_POLY1305
#define HAVE_POLY1305

#undef  HAVE_ONE_TIME_AUTH
#define HAVE_ONE_TIME_AUTH

#undef  HAVE_CHACHA
#define HAVE_CHACHA

#undef  HAVE_HASHDRBG
#define HAVE_HASHDRBG

#undef  HAVE_TLS_EXTENSIONS
#define HAVE_TLS_EXTENSIONS

#undef  HAVE_SNI
#define HAVE_SNI

#undef  HAVE_TLS_EXTENSIONS
#define HAVE_TLS_EXTENSIONS

#undef  HAVE_ALPN
#define HAVE_ALPN

#undef  HAVE_TLS_EXTENSIONS
#define HAVE_TLS_EXTENSIONS

#undef  HAVE_SUPPORTED_CURVES
#define HAVE_SUPPORTED_CURVES

#undef  HAVE_EXTENDED_MASTER
#define HAVE_EXTENDED_MASTER

#undef  WOLFSSL_TEST_CERT
#define WOLFSSL_TEST_CERT

#undef  NO_PSK
#define NO_PSK

#undef  NO_MD4
#define NO_MD4

#undef  USE_FAST_MATH
#define USE_FAST_MATH

#undef  WC_NO_ASYNC_THREADING
#define WC_NO_ASYNC_THREADING


#ifdef __cplusplus
}
#endif

#endif USER_SETTINGS_H