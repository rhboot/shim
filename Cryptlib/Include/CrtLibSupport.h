/** @file
  Root include file of C runtime library to support building the third-party
  cryptographic library.

Copyright (c) 2010 - 2022, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
Copyright (c) 2022, Loongson Technology Corporation Limited. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __CRT_LIB_SUPPORT_H__
#define __CRT_LIB_SUPPORT_H__

#if defined(__x86_64__)
/* shim.h will check if the compiler is new enough in some other CU */

#if !defined(GNU_EFI_USE_MS_ABI)
#define GNU_EFI_USE_MS_ABI
#endif

#if !defined(GNU_EFI_USE_COPYMEM_ABI)
#define GNU_EFI_USE_COPYMEM_ABI 0
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

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include "Library/MemoryAllocationLib.h"
#include "Library/DebugLib.h"

#define CONST const

#define OPENSSLDIR  ""
#define ENGINESDIR  ""
#define MODULESDIR  ""

#define MAX_STRING_SIZE  0x1000

//
// OpenSSL relies on explicit configuration for word size in crypto/bn,
// but we want it to be automatically inferred from the target. So we
// bypass what's in <openssl/opensslconf.h> for OPENSSL_SYS_UEFI, and
// define our own here.
//
#if defined(CONFIG_HEADER_BN_H) && !defined(OPENSSL_SYSNAME_UEFI)
#define CONFIG_HEADER_BN_H
#endif

#if !defined (SIXTY_FOUR_BIT) && !defined (THIRTY_TWO_BIT)
  #if defined (MDE_CPU_X64) || defined (MDE_CPU_AARCH64) || defined (MDE_CPU_IA64) || defined (MDE_CPU_RISCV64) || defined (MDE_CPU_LOONGARCH64)
//
// With GCC we would normally use SIXTY_FOUR_BIT_LONG, but MSVC needs
// SIXTY_FOUR_BIT, because 'long' is 32-bit and only 'long long' is
// 64-bit. Since using 'long long' works fine on GCC too, just do that.
//
#define SIXTY_FOUR_BIT
  #elif defined (MDE_CPU_IA32) || defined (MDE_CPU_ARM) || defined (MDE_CPU_EBC)
#define THIRTY_TWO_BIT
  #else
    #error Unknown target architecture
  #endif
#endif

//
// Definitions for global constants used by CRT library routines
//
#define ENOMEM       12               /* Cannot allocate memory */
#define EINVAL        22              /* Invalid argument */
#define BUFSIZ       1024             /* size of buffer used by setbuf */
#define EAFNOSUPPORT  47              /* Address family not supported by protocol family */
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
// Address families.
//
#define AF_INET   2     /* internetwork: UDP, TCP, etc. */
#define AF_INET6  24    /* IP version 6 */

//
// Define constants based on RFC0883, RFC1034, RFC 1035
//
#define NS_INT16SZ    2   /*%< #/bytes of data in a u_int16_t */
#define NS_INADDRSZ   4   /*%< IPv4 T_A */
#define NS_IN6ADDRSZ  16  /*%< IPv6 T_AAAA */

//
// Basic types mapping
//
typedef UINTN   off_t;
typedef INT64   time_t;
typedef UINT8   sa_family_t;
typedef UINT32  uid_t;
typedef UINT32  gid_t;

//
// File operations are not required for EFI building,
// so FILE is mapped to VOID * to pass build
//
typedef VOID *FILE;

//
// Structures Definitions
//
struct tm {
  int     tm_sec;    /* seconds after the minute [0-60] */
  int     tm_min;    /* minutes after the hour [0-59] */
  int     tm_hour;   /* hours since midnight [0-23] */
  int     tm_mday;   /* day of the month [1-31] */
  int     tm_mon;    /* months since January [0-11] */
  int     tm_year;   /* years since 1900 */
  int     tm_wday;   /* days since Sunday [0-6] */
  int     tm_yday;   /* days since January 1 [0-365] */
  int     tm_isdst;  /* Daylight Savings Time flag */
  long    tm_gmtoff; /* offset from CUT in seconds */
  char    *tm_zone;  /* timezone abbreviation */
};

struct timeval {
  long    tv_sec;   /* time value, in seconds */
  long    tv_usec;  /* time value, in microseconds */
};

struct dirent {
  UINT32  d_fileno;         /* file number of entry */
  UINT16  d_reclen;         /* length of this record */
  UINT8   d_type;           /* file type, see below */
  UINT8   d_namlen;         /* length of string in d_name */
  char    d_name[255 + 1];  /* name must be no longer than this */
};

/*
 * These are probably both wrong *and* useless...
 */
typedef uint64_t ino_t;
typedef uint64_t dev_t;
typedef uint64_t mode_t;
typedef uint64_t nlink_t;

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

struct timezone;

struct sockaddr {
  uint8_t      sa_len;      /* total length */
  sa_family_t    sa_family;   /* address family */
  char           sa_data[14]; /* actually longer; address value */
};

//
// Global variables
//
extern int   errno;
extern FILE  *stderr;
extern FILE  *stdin;
extern FILE  *stdout;
extern long  timezone;

//
// Function prototypes of CRT Library routines
//
void           *malloc     (size_t);
void           *realloc    (void *, size_t);
void           free        (void *);
void           *memset     (void *, int, size_t);
int            memcmp      (const void *, const void *, size_t);
char           *strcpy     (char *, const char *);
char           *strncpy    (char *, const char *, size_t);
char           *strcat     (char *, const char *);
int            strcmp      (const char *, const char *);
int            strcasecmp  (const char *, const char *);
char           *strchr     (const char *, int);
char           *strrchr    (const char *, int);
unsigned long  strtoul     (const char *, char **, int);
long           strtol      (const char *, char **, int);
char           *strerror   (int);
size_t         strspn      (const char *, const char *);
size_t         strcspn     (const char *, const char *);
int            printf      (const char *, ...);
int            sscanf      (const char *, const char *, ...);
FILE           *fopen      (const char *, const char *);
size_t         fread       (void *, size_t, size_t, FILE *);
size_t         fwrite      (const void *, size_t, size_t, FILE *);
FILE           *fopen      (const char *, const char *);
size_t         fread       (void *, size_t, size_t, FILE *);
size_t         fwrite      (const void *, size_t, size_t, FILE *);
int            fclose      (FILE *);
int            fprintf     (FILE *, const char *, ...);
struct tm      *gmtime     (const time_t *);
struct tm      *gmtime_r   (const time_t *, struct tm *);
unsigned int   sleep       (unsigned int  seconds);
int            gettimeofday(struct timeval   *tv, struct timezone  *tz);
time_t         mktime      (struct tm  *t);
uid_t          getuid      (void);
uid_t          geteuid     (void);
gid_t          getgid      (void);
gid_t          getegid     (void);
int            issetugid   (void);
void           qsort       (void *, size_t, size_t, int (*)(const void *, const void *));
char           *getenv     (const char *);
char           *secure_getenv(const char *);
int            inet_pton   (int, const char *, void *);

INT64
EFIAPI
DivS64x64Remainder (
  IN      INT64  Dividend,
  IN      INT64  Divisor,
  OUT     INT64  *Remainder  OPTIONAL
  );

void *memcpy     (void *, const void *, size_t);
void setbuf(FILE *, char *buffer);

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
