/** @file
  Root include file to support building OpenSSL Crypto Library.

Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __OPEN_SSL_SUPPORT_H__
#define __OPEN_SSL_SUPPORT_H__

#if defined(__x86_64__)
/* shim.h will check if the compiler is new enough in some other CU */

#if !defined(GNU_EFI_USE_EXTERNAL_STDARG)
#define GNU_EFI_USE_EXTERNAL_STDARG
#endif

#if !defined(GNU_EFI_USE_MS_ABI)
#define GNU_EFI_USE_MS_ABI
#endif

#ifdef NO_BUILTIN_VA_FUNCS
#undef NO_BUILTIN_VA_FUNCS
#endif
#endif

/*
 * Include stddef.h to avoid redefining "offsetof"
 */
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include <efi.h>
#include <efilib.h>
#include "Base.h"
#include "Library/BaseLib.h"
#include "Library/BaseMemoryLib.h"
#include "Library/MemoryAllocationLib.h"
#include "Library/DebugLib.h"

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
// #defines from EFI Application Toolkit required to buiild Open SSL
//
#define ENOMEM       12               /* Cannot allocate memory */
#define EINVAL       22               /* Invalid argument */
#define BUFSIZ       1024             /* size of buffer used by setbuf */
#define INT_MAX      2147483647       /* max value for an int */
#define INT_MIN      (-2147483647-1)  /* min value for an int */
#define LONG_MAX     2147483647L      /* max value for a long */
#define LONG_MIN     (-2147483647-1)  /* min value for a long */
#define ULONG_MAX    0xffffffff       /* max value for an unsigned long */
#define LOG_DAEMON   (3<<3)           /* system daemons */
#define LOG_EMERG    0                /* system is unusable */
#define LOG_ALERT    1                /* action must be taken immediately */
#define LOG_CRIT     2                /* critical conditions */
#define LOG_ERR      3                /* error conditions */
#define LOG_WARNING  4                /* warning conditions */
#define LOG_NOTICE   5                /* normal but significant condition */
#define LOG_INFO     6                /* informational */
#define LOG_DEBUG    7                /* debug-level messages */
#define LOG_PID      0x01             /* log the pid with each message */
#define LOG_CONS     0x02             /* log on the console if errors in sending */

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

//
// Basic types from EFI Application Toolkit required to buiild Open SSL
//
typedef UINTN          size_t;
typedef INTN           ssize_t;
typedef INT64          off_t;
typedef UINT16         mode_t;
typedef long           time_t;
typedef unsigned long  clock_t;
typedef UINT32         uid_t;
typedef UINT32         gid_t;
typedef UINT32         ino_t;
typedef UINT32         dev_t;
typedef UINT16         nlink_t;
typedef int            pid_t;
typedef void           *DIR;
typedef void           __sighandler_t (int);

//
// Structures from EFI Application Toolkit required to buiild Open SSL
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
// Externs from EFI Application Toolkit required to buiild Open SSL
//
extern int errno;

//
// Function prototypes from EFI Application Toolkit required to buiild Open SSL
//
void           *malloc     (size_t);
void           *realloc    (void *, size_t);
void           free        (void *);
void           *memcpy     (void *, const void *, size_t);
void           *memset     (void *, int, size_t);
void           *memchr     (const void *, int, size_t);
int            memcmp      (const void *, const void *, size_t);
void           *memmove    (void *, const void *, size_t);
char           *strcpy     (char *, const char *);
char           *strncpy    (char *, const char *, size_t);
char           *strcat     (char *, const char *);
char           *strchr     (const char *, int);
int            strcasecmp  (const char *, const char *);
int            strncasecmp (const char *, const char *, size_t);
char           *strncpy    (char *, const char *, size_t);
unsigned long  strtoul     (const char *, char **, int);
long           strtol      (const char *, char **, int);
char           *strerror   (int);
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
char           *fgets      (char *, int, FILE *);
int            fputs       (const char *, FILE *);
int            fprintf     (FILE *, const char *, ...);
int            vfprintf    (FILE *, const char *, ms_va_list);
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
void           qsort       (void *, size_t, size_t, int (*)(const void *, const void *));
char           *getenv     (const char *);
void           exit        (int);
__sighandler_t *signal     (int, __sighandler_t *);

//
// Global variables from EFI Application Toolkit required to buiild Open SSL
//
extern FILE  *stderr;
extern FILE  *stdin;
extern FILE  *stdout;

#define AsciiStrLen(x) strlen(x)
#define AsciiStrnCmp(s1, s2, len) strncmpa((CHAR8 *)s1, (CHAR8 *)s2, len)

//
// Macros that directly map functions to BaseLib, BaseMemoryLib, and DebugLib functions
//
#define memcpy(dest,source,count)         ( {CopyMem(dest,(void *)source,(UINTN)(count)); dest; })
#define memset(dest,ch,count)             SetMem(dest,(UINTN)(count),(UINT8)(ch))
#define memchr(buf,ch,count)              ScanMem8((CHAR8 *)buf,(UINTN)(count),ch)
#define memcmp(buf1,buf2,count)           (int)(CompareMem(buf1,buf2,(UINTN)(count)))
#define memmove(dest,source,count)        CopyMem(dest,(void *)source,(UINTN)(count))
#define localtime(timer)                  NULL
#define assert(expression)
#define atoi(nptr)                        AsciiStrDecimalToUintn((const CHAR8 *)nptr)
#define gettimeofday(tvp,tz)              do { (tvp)->tv_sec = time(NULL); (tvp)->tv_usec = 0; } while (0)
#define gmtime_r(timer,result)            (result = NULL)

#endif
