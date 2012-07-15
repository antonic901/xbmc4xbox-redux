
#ifndef XBOX_NO_XBP_DEFINE

#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <sys/stat.h>

#ifndef _XBOX
#define _XBOX
#endif

// XBOX file imports
#pragma comment (lib, "xbox/xbp.lib")

//__declspec(dllimport) int xbp_fgetc(FILE *stream);
//#define fgetc(a) xbp_fgetc(a)

//__declspec(dllimport) void xbp_clearerr(FILE *stream);
//#define clearerr(a) xbp_clearerr(a)

//__declspec(dllimport) int xbp__stati64(const char *path, struct _stati64 *buffer);
//#define _stati64(a, b) xbp__stati64(a, b)

//__declspec(dllimport) int xbp__fstati64(int fd, struct _stati64 *buffer);
//#define _fstati64(a, b) xbp__fstati64(a, b)

__declspec(dllimport) char *xbp__tempnam(const char *dir, const char *prefix);
#define _tempnam(a, b) xbp__tempnam(a, b)

__declspec(dllimport) FILE *xbp_tmpfile(void);
 #define tmpfile() xbp_tmpfile()
 
__declspec(dllimport) char *xbp_tmpnam(char *string);
 #define tmpnam(a) xbp_tmpnam(a)

__declspec(dllimport) FILE *xbp_fopen(const char *filename, const char *mode);
 #define fopen(a, b) xbp_fopen(a, b)

__declspec(dllimport) FILE *xbp__wfopen(const wchar_t *filename, const wchar_t *mode);
#define _wfopen(a, b) xbp__wfopen(a, b)

__declspec(dllimport) long xbp__get_osfhandle(int fd);
#define _get_osfhandle(a) xbp__get_osfhandle(a)

//__declspec(dllimport) int xbp_vfprintf(FILE *stream, const char *format, va_list argptr);
//#define vfprintf(a, b, c) xbp_vfprintf(a, b, c)

__declspec(dllimport) int xbp_unlink(const char *filename);
#define unlink xbp_unlink

__declspec(dllimport) int xbp_access(const char *path, int mode);
#define access(a, b) xbp_access(a, b)

__declspec(dllimport) int xbp_chdir(const char *dirname);
#define chdir xbp_chdir

__declspec(dllimport) int xbp_chmod(const char *filename, int pmode);
#define chmod(a, b) xbp_chmod(a, b)

__declspec(dllimport) int xbp_rmdir(const char *dirname);
#define rmdir xbp_rmdir

__declspec(dllimport) int xbp_umask(int pmode);
#define umask(a) xbp_umask(a)

__declspec(dllimport) int xbp_utime(const char *filename, struct utimbuf *times);
#define utime(a, b) xbp_utime(a, b)

__declspec(dllimport) int xbp_dup(int fd);
#define dup(a) xbp_dup(a)

__declspec(dllimport) int xbp_dup2(int fd1, int fd2);
#define dup2(a, b) xbp_dup2(a, b)

__declspec(dllimport) int xbp_rename(const char *oldname, const char *newname);
#define rename xbp_rename

__declspec(dllimport) int xbp__commit(int fd);
#define _commit(a) xbp__commit(a)

__declspec(dllimport) int xbp__setmode(int fd, int mode);
#define _setmode(a, b) xbp__setmode(a, b)

//__declspec(dllimport) int xbp_ungetc(int c, FILE *stream);
//#define ungetc(a, b) xbp_ungetc(a, b)

//__declspec(dllimport) FILE *xbp_fopen(const char *filename, const char *mode);
#define fopen(a, b) xbp_fopen(a, b)

//__declspec(dllimport) int xbp_fflush(FILE *stream);
//#define fflush(a) xbp_fflush(a)

//__declspec(dllimport) size_t xbp_fwrite(const void *buffer, size_t size, size_t count, FILE *stream);
//#define fwrite(a, b, c, d) xbp_fwrite(a, b, c, d)

//__declspec(dllimport) int xbp_fprintf(FILE *stream, const char *format, ...);
//#define fprintf xbp_fprintf

//__declspec(dllimport) size_t xbp_fread(void *buffer, size_t size, size_t count, FILE *stream);
//#define fread(a, b, c, d) xbp_fread(a, b, c, d)

//__declspec(dllimport) int xbp_fputs(const char *string, FILE *stream);
//#define fputs(a, b) xbp_fputs(a, b)

//__declspec(dllimport) int xbp_getc(FILE *stream);
//#define getc(a) xbp_getc(a)

__declspec(dllimport) int xbp_setvbuf(FILE *stream, char *buffer, int mode, size_t size);
#define setvbuf(a, b, c, d) xbp_setvbuf(a, b, c, d)

//__declspec(dllimport) int xbp_fsetpos(FILE *stream, const fpos_t *pos);
//#define fsetpos(a, b) xbp_fsetpos(a, b)

//__declspec(dllimport) int xbp_fgetpos(FILE *stream, const fpos_t *pos);
//#define fgetpos(a, b) xbp_fgetpos(a, b)

//__declspec(dllimport) long xbp__lseek(int fd, long offset, int origin);
//#define _lseek(a, b, c) xbp__lseek(a, b, c)

//__declspec(dllimport) long xbp_ftell(FILE *stream);
//#define ftell(a) xbp_ftell(a)

//__declspec(dllimport) long xbp_ftell64(FILE *stream);
//#define ftell64(a) xbp_ftell64(a)

//__declspec(dllimport) char *xbp_fgets(char *string, int n, FILE *stream);
//#define fgets(a, b, c) xbp_fgets(a, b, c)

//__declspec(dllimport) int xbp_fseek(FILE *stream, long offset, int origin);
//#define fseek(a, b, c) xbp_fseek(a, b, c)

//__declspec(dllimport) int xbp_putc(int c, FILE *stream);
//#define putc(a, b) xbp_putc(a, b)

//__declspec(dllimport) int xbp_write(int fd, const void *buffer, unsigned int count);
//#define write(a, b, c) xbp_write(a, b, c)

//__declspec(dllimport) int xbp_read(int fd, void *buffer, unsigned int count);
//#define read(a, b, c) xbp_read(a, b, c)

__declspec(dllimport) int xbp_close(int fd);
#define close(a) xbp_close(a)

//__declspec(dllimport) int xbp_mkdir(const char *dirname);
//#define mkdir(a) xbp_mkdir(a)

__declspec(dllimport) int xbp_open(const char *filename, int oflag, int pmode);
#define open(a, b, c) xbp_open(a, b, c)

//__declspec(dllimport) FILE * xbp__fdopen(int fd, const char *mode);
//#define fdopen(a, b) xbp__fdopen(a, b)

//__declspec(dllimport) long xbp_lseek(int fd, long offset, int origin);
//#define lseek(a, b, c) xbp_lseek(a, b, c)

//__declspec(dllimport) int xbp_fstat(int fd, struct stat *buffer);
//#define fstat(a, b) xbp_fstat(a, b)

//__declspec(dllimport) int xbp_stat(const char *path, struct stat *buffer);
//#define stat(a, b) xbp_stat(a, b)

//__declspec(dllimport) __int64 xbp__lseeki64(int fd, __int64 offset, int origin);
//#define _lseeki64(a, b, c) xbp__lseeki64(a, b, c)

//__declspec(dllimport) int xbp_fileno(FILE *stream);
//#define fileno(a) xbp_fileno(a)

//__declspec(dllimport) int xbp_fputc(int c, FILE *stream);
//#define fputc(a, b) xbp_fputc(a, b)

//__declspec(dllimport) void xbp_rewind(FILE *stream);
//#define rewind(a) xbp_rewind(a)

__declspec(dllimport) int xbp_fclose(FILE *stream);
#define fclose xbp_fclose

__declspec(dllimport) int xbp_isatty(int fd);
#define isatty(a) xbp_isatty(a)

__declspec(dllimport) char *xbp_getcwd(char *buffer, int maxlen);
#define getcwd(a, b) xbp_getcwd(a, b)

#undef FindClose
__declspec(dllimport) BOOL xbp_FindClose(HANDLE hFindFile);
#define FindClose(a) xbp_FindClose(a)

#undef FindFirstFile
__declspec(dllimport) HANDLE xbp_FindFirstFile(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData);
#define FindFirstFile(a, b) xbp_FindFirstFile(a, b)

#undef FindNextFile
__declspec(dllimport) BOOL xbp_FindNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData);
#define FindNextFile(a, b) xbp_FindNextFile(a, b)

//#undef getenv
//__declspec(dllimport) char* xbp_getenv(const char* s);
//#define getenv(s) xbp_getenv(s)

//#undef putenv
//__declspec(dllimport) int xbp_putenv(const char* s);
//#define putenv(s) xbp_putenv(s)

#endif // XBOX_NO_XBP_DEFINE