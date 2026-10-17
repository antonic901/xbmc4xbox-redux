/* config.h - FriBidi 1.0.17 */

#ifndef FRIBIDI_BUILD_CONFIG_H
#define FRIBIDI_BUILD_CONFIG_H

#ifdef _DEBUG
#define DEBUG 1
#endif

/* Keep deprecated functionality enabled */
//undef FRIBIDI_NO_DEPRECATED

/* Available headers */
#define HAVE_INTTYPES_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_WCHAR_H 1

/* Available functions */
#define HAVE_MEMMOVE 1
#define HAVE_MEMSET 1
#define HAVE_STRDUP 1

/* ANSI C preprocessor support */
#define HAVE_STRINGIZE 1

/* Package information */
#define PACKAGE_BUGREPORT "https://github.com/fribidi/fribidi/issues"
#define PACKAGE_NAME "GNU FriBidi"
#define PACKAGE_STRING "GNU FriBidi 1.0.17"
#define PACKAGE_TARNAME "fribidi"
#define PACKAGE_URL "https://github.com/fribidi/fribidi"
#define PACKAGE_VERSION "1.0.17"

/* Signal handler return type */
#define RETSIGTYPE void

/* Type sizes */
#define SIZEOF_INT 4
#define SIZEOF_SHORT 2
#define SIZEOF_VOID_P 4
#define SIZEOF_WCHAR_T 2

/* ANSI C headers */
#define STDC_HEADERS 1

#endif /* FRIBIDI_BUILD_CONFIG_H */
