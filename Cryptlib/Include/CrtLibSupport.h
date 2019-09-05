/** @file
  Root include file of C runtime library to support building the third-party
  cryptographic library.

Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __CRT_LIB_SUPPORT_H__
#define __CRT_LIB_SUPPORT_H__

#include "Base.h"
#include "Library/BaseLib.h"
#include "Library/BaseMemoryLib.h"
#include "Library/MemoryAllocationLib.h"
#include "Library/DebugLib.h"

/*
 * Include stddef.h to avoid redefining "offsetof"
 */
#include <stddef.h>

#define OPENSSLDIR ""
#define ENGINESDIR ""

#define MAX_STRING_SIZE  0x1000

//
// We already have "no-ui" in out Configure invocation.
// but the code still fails to compile.
// Ref:  https://github.com/openssl/openssl/issues/8904
//
// This is defined in CRT library(stdio.h).
//
#ifndef BUFSIZ
#define BUFSIZ  8192
#endif

#define CONST const

//
// OpenSSL relies on explicit configuration for word size in crypto/bn,
// but we want it to be automatically inferred from the target. So we
// bypass what's in <openssl/opensslconf.h> for OPENSSL_SYS_UEFI, and
// define our own here.
//
#ifdef CONFIG_HEADER_BN_H
#error CONFIG_HEADER_BN_H already defined
#endif

#define CONFIG_HEADER_BN_H

#if defined(MDE_CPU_X64) || defined(MDE_CPU_AARCH64) || defined(MDE_CPU_IA64)
//
// With GCC we would normally use SIXTY_FOUR_BIT_LONG, but MSVC needs
// SIXTY_FOUR_BIT, because 'long' is 32-bit and only 'long long' is
// 64-bit. Since using 'long long' works fine on GCC too, just do that.
//
#define SIXTY_FOUR_BIT
#elif defined(MDE_CPU_IA32) || defined(MDE_CPU_ARM) || defined(MDE_CPU_EBC)
#define THIRTY_TWO_BIT
#else
#error Unknown target architecture
#endif

//
// File operations are not required for building Open SSL,
// so FILE is mapped to VOID * to pass build
//
typedef VOID  *FILE;

//
// Definitions for global constants used by CRT library routines
//
#define EINVAL       22               /* Invalid argument */
#define INT_MAX      0x7FFFFFFF       /* Maximum (signed) int value */
#define LONG_MAX     0X7FFFFFFFL      /* max value for a long */
#define LONG_MIN     (-LONG_MAX-1)    /* min value for a long */
#define ULONG_MAX    0xFFFFFFFF       /* Maximum unsigned long value */
#define CHAR_BIT     8                /* Number of bits in a char */

//
// Macros from EFI Application Toolkit required to buiild Open SSL
//
/* The offsetof() macro calculates the offset of a structure member
   in its structure.  Unfortunately this cannot be written down
   portably, hence it is provided by a Standard C header file.
   For pre-Standard C compilers, here is a version that usually works
   (but watch out!): */
#ifndef offsetof
#define offsetof(TYPE, MEMBER) __builtin_offsetof (TYPE, MEMBER)
#endif

typedef UINTN RETURN_STATUS;

#define MAX_BIT     (1ULL << (sizeof (INTN) * 8 - 1))

#define ENCODE_ERROR(StatusCode)     ((RETURN_STATUS)(MAX_BIT | (StatusCode)))
#define ENCODE_WARNING(StatusCode)   ((RETURN_STATUS)(StatusCode))
#define RETURN_ERROR(StatusCode)     (((INTN)(RETURN_STATUS)(StatusCode)) < 0)

#define RETURN_SUCCESS 0
#define RETURN_LOAD_ERROR            ENCODE_ERROR (1)
#define RETURN_INVALID_PARAMETER     ENCODE_ERROR (2)
#define RETURN_UNSUPPORTED           ENCODE_ERROR (3)
#define RETURN_BAD_BUFFER_SIZE       ENCODE_ERROR (4)
#define RETURN_BUFFER_TOO_SMALL      ENCODE_ERROR (5)
#define RETURN_NOT_READY             ENCODE_ERROR (6)
#define RETURN_DEVICE_ERROR          ENCODE_ERROR (7)
#define RETURN_WRITE_PROTECTED       ENCODE_ERROR (8)
#define RETURN_OUT_OF_RESOURCES      ENCODE_ERROR (9)
#define RETURN_VOLUME_CORRUPTED      ENCODE_ERROR (10)
#define RETURN_VOLUME_FULL           ENCODE_ERROR (11)
#define RETURN_NO_MEDIA              ENCODE_ERROR (12)
#define RETURN_MEDIA_CHANGED         ENCODE_ERROR (13)
#define RETURN_NOT_FOUND             ENCODE_ERROR (14)
#define RETURN_ACCESS_DENIED         ENCODE_ERROR (15)
#define RETURN_NO_RESPONSE           ENCODE_ERROR (16)
#define RETURN_NO_MAPPING            ENCODE_ERROR (17)
#define RETURN_TIMEOUT               ENCODE_ERROR (18)
#define RETURN_NOT_STARTED           ENCODE_ERROR (19)
#define RETURN_ALREADY_STARTED       ENCODE_ERROR (20)
#define RETURN_ABORTED               ENCODE_ERROR (21)
#define RETURN_ICMP_ERROR            ENCODE_ERROR (22)
#define RETURN_TFTP_ERROR            ENCODE_ERROR (23)
#define RETURN_PROTOCOL_ERROR        ENCODE_ERROR (24)
#define RETURN_INCOMPATIBLE_VERSION  ENCODE_ERROR (25)
#define RETURN_SECURITY_VIOLATION    ENCODE_ERROR (26)
#define RETURN_CRC_ERROR             ENCODE_ERROR (27)
#define RETURN_END_OF_MEDIA          ENCODE_ERROR (28)
#define RETURN_END_OF_FILE           ENCODE_ERROR (31)
#define RETURN_INVALID_LANGUAGE      ENCODE_ERROR (32)
#define RETURN_COMPROMISED_DATA      ENCODE_ERROR (33)
#define RETURN_HTTP_ERROR            ENCODE_ERROR (35)

#define RETURN_WARN_UNKNOWN_GLYPH    ENCODE_WARNING (1)
#define RETURN_WARN_DELETE_FAILURE   ENCODE_WARNING (2)
#define RETURN_WARN_WRITE_FAILURE    ENCODE_WARNING (3)
#define RETURN_WARN_BUFFER_TOO_SMALL ENCODE_WARNING (4)
#define RETURN_WARN_STALE_DATA       ENCODE_WARNING (5)
#define RETURN_WARN_FILE_SYSTEM      ENCODE_WARNING (6)

#define SIGNATURE_16(A, B)        ((A) | (B << 8))
#define SIGNATURE_32(A, B, C, D)  (SIGNATURE_16 (A, B) | (SIGNATURE_16 (C, D) << 16))

//
// Basic types mapping
//
typedef UINTN          size_t;
typedef INTN           ssize_t;
typedef INT32          time_t;
typedef UINT8          __uint8_t;
typedef UINT8          sa_family_t;
typedef UINT32         uid_t;
typedef UINT32         gid_t;

//
// File operations are not required for EFI building,
// so FILE is mapped to VOID * to pass build
//
typedef VOID  *FILE;
typedef INT64          off_t;
typedef UINT16         mode_t;
typedef unsigned long  clock_t;
typedef UINT32         ino_t;
typedef UINT32         dev_t;
typedef UINT16         nlink_t;
typedef int            pid_t;
typedef void           *DIR;

//
// Structures Definitions
//
struct tm {
  int   tm_sec;     /* seconds after the minute [0-60] */
  int   tm_min;     /* minutes after the hour [0-59] */
  int   tm_hour;    /* hours since midnight [0-23] */
  int   tm_mday;    /* day of the month [1-31] */
  int   tm_mon;     /* months since January [0-11] */
  int   tm_year;    /* years since 1900 */
  int   tm_wday;    /* days since Sunday [0-6] */
  int   tm_yday;    /* days since January 1 [0-365] */
  int   tm_isdst;   /* Daylight Savings Time flag */
  long  tm_gmtoff;  /* offset from CUT in seconds */
  char  *tm_zone;   /* timezone abbreviation */
};

struct timeval {
  long tv_sec;      /* time value, in seconds */
  long tv_usec;     /* time value, in microseconds */
};

struct sockaddr {
  __uint8_t    sa_len;       /* total length */
  sa_family_t  sa_family;    /* address family */
  char         sa_data[14];  /* actually longer; address value */
};

struct dirent {
  UINT32  d_fileno;         /* file number of entry */
  UINT16  d_reclen;         /* length of this record */
  UINT8   d_type;           /* file type, see below */
  UINT8   d_namlen;         /* length of string in d_name */
  char    d_name[255 + 1];  /* name must be no longer than this */
};

struct stat {
  dev_t    st_dev;          /* inode's device */
  ino_t    st_ino;          /* inode's number */
  mode_t   st_mode;         /* inode protection mode */
  nlink_t  st_nlink;        /* number of hard links */
  uid_t    st_uid;          /* user ID of the file's owner */
  gid_t    st_gid;          /* group ID of the file's group */
  dev_t    st_rdev;         /* device type */
  time_t   st_atime;        /* time of last access */
  long     st_atimensec;    /* nsec of last access */
  time_t   st_mtime;        /* time of last data modification */
  long     st_mtimensec;    /* nsec of last data modification */
  time_t   st_ctime;        /* time of last file status change */
  long     st_ctimensec;    /* nsec of last file status change */
  off_t    st_size;         /* file size, in bytes */
  INT64    st_blocks;       /* blocks allocated for file */
  UINT32   st_blksize;      /* optimal blocksize for I/O */
  UINT32   st_flags;        /* user defined flags for file */
  UINT32   st_gen;          /* file generation number */
  INT32    st_lspare;
  INT64    st_qspare[2];
};

//
// Global variables
//
extern int  errno;
extern FILE *stderr;

//
// Function prototypes of CRT Library routines
//
void           *malloc     (size_t);
void           *realloc    (void *, size_t);
void           free        (void *);
void           *memcpy     (void *, const void *, size_t);
void           *memchr     (const void *, int, size_t);
int            memcmp      (const void *, const void *, size_t);
void           *memmove    (void *, const void *, size_t);
void           *memset     (void *, int, size_t);
int            isdigit     (int);
int            isspace     (int);
int            isxdigit    (int);
int            isalnum     (int);
int            isupper     (int);
int            tolower     (int);
int            strcmp      (const char *, const char *);
int            strncmp     (const char *, const char *, size_t);
int            strncasecmp (const char *, const char *, size_t);
int            strcasecmp  (const char *, const char *);
char           *strcpy     (char *, const char *);
char           *strncpy    (char *, const char *, size_t);
size_t         strlen      (const char *);
char           *strcat     (char *, const char *);
char           *strchr     (const char *, int);
char           *strncpy    (char *, const char *, size_t);
char           *strrchr    (const char *, int);
unsigned long  strtoul     (const char *, char **, int);
long           strtol      (const char *, char **, int);
char           *strerror   (int);
size_t         strspn      (const char *, const char *);
size_t         strcspn     (const char *, const char *);
int            printf      (const char *, ...);
int            sscanf      (const char *, const char *, ...);
int            open        (const char *, int, ...);
int            chmod       (const char *, mode_t);
int            stat        (const char *, struct stat *);
off_t          lseek       (int, off_t, int);
ssize_t        read        (int, void *, size_t);
ssize_t        write       (int, const void *, size_t);
int            close       (int);
FILE           *fopen      (const char *, const char *);
size_t         fread       (void *, size_t, size_t, FILE *);
size_t         fwrite      (const void *, size_t, size_t, FILE *);
int            fclose      (FILE *);
char           *fgets      (char *, int, FILE *);
int            fputs       (const char *, FILE *);
int            fprintf     (FILE *, const char *, ...);
int            vfprintf    (FILE *, const char *, VA_LIST);
int            fflush      (FILE *);
int            fclose      (FILE *);
DIR            *opendir    (const char *);
struct dirent  *readdir    (DIR *);
int            closedir    (DIR *);
void           openlog     (const char *, int, int);
void           closelog    (void);
void           syslog      (int, const char *, ...);
time_t         time        (time_t *);
struct tm      *localtime  (const time_t *);
struct tm      *gmtime     (const time_t *);
struct tm      *gmtime_r   (const time_t *, struct tm *);
uid_t          getuid      (void);
uid_t          geteuid     (void);
gid_t          getgid      (void);
gid_t          getegid     (void);
int            issetugid   (void);
void           qsort       (void *, size_t, size_t, int (*)(const void *, const void *));
char           *getenv     (const char *);
char           *secure_getenv (const char *);
void           exit        (int);
#if defined(__GNUC__) && (__GNUC__ >= 2)
void           abort       (void) __attribute__((__noreturn__));
#else
void           abort       (void);
#endif

//
// Global variables from EFI Application Toolkit required to buiild Open SSL
//
extern FILE  *stderr;
extern FILE  *stdin;
extern FILE  *stdout;

#define AsciiStrLen(x) AsciiStrnLenS(x,MAX_STRING_SIZE)
#define AsciiStrnCmp(s1, s2, len) strncmpa((CHAR8 *)s1, (CHAR8 *)s2, len)

//
// Macros that directly map functions to BaseLib, BaseMemoryLib, and DebugLib functions
//
#define memcpy(dest,source,count)         ( {CopyMem(dest,source,(UINTN)(count)); dest; })
#define memset(dest,ch,count)             SetMem(dest,(UINTN)(count),(UINT8)(ch))
#define memchr(buf,ch,count)              ScanMem8((CHAR8 *)buf,(UINTN)(count),ch)
#define memcmp(buf1,buf2,count)           (int)(CompareMem(buf1,buf2,(UINTN)(count)))
#define memmove(dest,source,count)        CopyMem(dest,source,(UINTN)(count))
#define strlen(str)                       (size_t)(AsciiStrnLenS(str,MAX_STRING_SIZE))
#define strcpy(strDest,strSource)         AsciiStrCpyS(strDest,MAX_STRING_SIZE,strSource)
#define strncpy(strDest,strSource,count)  AsciiStrnCpyS(strDest,MAX_STRING_SIZE,strSource,(UINTN)count)
#define strcat(strDest,strSource)         AsciiStrCatS(strDest,MAX_STRING_SIZE,strSource)
#define stpcpy(strDest,StrSource)         AsciiStpCpy(strDest,StrSource)
#define strchr(str,ch)                    ScanMem8((VOID *)(str),AsciiStrSize(str),(UINT8)ch)
#define strncmp(string1,string2,count)    (int)(AsciiStrnCmp(string1,string2,(UINTN)(count)))
#define localtime(timer)                  NULL
#define assert(expression)
#define atoi(nptr)                        AsciiStrDecimalToUintn(nptr)
#define gettimeofday(tvp,tz)              do { (tvp)->tv_sec = time(NULL); (tvp)->tv_usec = 0; } while (0)
#define gmtime_r(timer,result)            (result = NULL)

void clear_ca_warning();
BOOLEAN get_ca_warning();

#endif
