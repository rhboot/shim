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

#include <efi.h>
#include <efilib.h>
#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>

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
// Map all va_xxxx elements to VA_xxx defined in MdePkg/Include/Base.h
//
#if !defined(__CC_ARM) || defined(_STDARG_H) // if va_list is not already defined
/*
 * These are now unconditionally #defined by GNU_EFI's efistdarg.h,
 * so we should #undef them here before providing a new definition.
 */
#undef va_arg
#undef va_start
#undef va_end

#define va_list   VA_LIST
#define va_arg    VA_ARG
#define va_start  VA_START
#define va_end    VA_END

# if !defined(NO_BUILTIN_VA_FUNCS)

typedef __builtin_va_list VA_LIST;

#define VA_START(Marker, Parameter)  __builtin_va_start (Marker, Parameter)

#define VA_ARG(Marker, TYPE)         ((sizeof (TYPE) < sizeof (UINTN)) ? (TYPE)(__builtin_va_arg (Marker, UINTN)) : (TYPE)(__builtin_va_arg (Marker, TYPE)))

#define VA_END(Marker)               __builtin_va_end (Marker)

#define VA_COPY(Dest, Start)         __builtin_va_copy (Dest, Start)

# else

#define _INT_SIZE_OF(n) ((sizeof (n) + sizeof (UINTN) - 1) &~(sizeof (UINTN) - 1))
///
/// Variable used to traverse the list of arguments. This type can vary by
/// implementation and could be an array or structure.
///
typedef CHAR8 *VA_LIST;

/**
  Retrieves a pointer to the beginning of a variable argument list, based on
  the name of the parameter that immediately precedes the variable argument list.

  This function initializes Marker to point to the beginning of the variable
  argument list that immediately follows Parameter.  The method for computing the
  pointer to the next argument in the argument list is CPU-specific following the
  EFIAPI ABI.

  @param   Marker       The VA_LIST used to traverse the list of arguments.
  @param   Parameter    The name of the parameter that immediately precedes
                        the variable argument list.

  @return  A pointer to the beginning of a variable argument list.

**/
#define VA_START(Marker, Parameter) (Marker = (VA_LIST) ((UINTN) & (Parameter) + _INT_SIZE_OF (Parameter)))

/**
  Returns an argument of a specified type from a variable argument list and updates
  the pointer to the variable argument list to point to the next argument.

  This function returns an argument of the type specified by TYPE from the beginning
  of the variable argument list specified by Marker.  Marker is then updated to point
  to the next argument in the variable argument list.  The method for computing the
  pointer to the next argument in the argument list is CPU-specific following the EFIAPI ABI.

  @param   Marker   VA_LIST used to traverse the list of arguments.
  @param   TYPE     The type of argument to retrieve from the beginning
                    of the variable argument list.

  @return  An argument of the type specified by TYPE.

**/
#define VA_ARG(Marker, TYPE)   (*(TYPE *) ((Marker += _INT_SIZE_OF (TYPE)) - _INT_SIZE_OF (TYPE)))

/**
  Terminates the use of a variable argument list.

  This function initializes Marker so it can no longer be used with VA_ARG().
  After this macro is used, the only way to access the variable argument list is
  by using VA_START() again.

  @param   Marker   VA_LIST used to traverse the list of arguments.

**/
#define VA_END(Marker)      (Marker = (VA_LIST) 0)

/**
  Initializes a VA_LIST as a copy of an existing VA_LIST.

  This macro initializes Dest as a copy of Start, as if the VA_START macro had been applied to Dest
  followed by the same sequence of uses of the VA_ARG macro as had previously been used to reach
  the present state of Start.

  @param   Dest   VA_LIST used to traverse the list of arguments.
  @param   Start  VA_LIST used to traverse the list of arguments.

**/
#define VA_COPY(Dest, Start)  ((void)((Dest) = (Start)))

# endif

#else // __CC_ARM
#define va_start(Marker, Parameter)   __va_start(Marker, Parameter)
#define va_arg(Marker, TYPE)          __va_arg(Marker, TYPE)
#define va_end(Marker)                ((void)0)
#endif

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
#define offsetof(type, member) ( (int) & ((type*)0) -> member )
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
int            isdigit     (int);
int            isspace     (int);
int            tolower     (int);
int            isupper     (int);
int            isxdigit    (int);
int            isalnum     (int);
void           *memcpy     (void *, const void *, size_t);
void           *memset     (void *, int, size_t);
void           *memchr     (const void *, int, size_t);
int            memcmp      (const void *, const void *, size_t);
void           *memmove    (void *, const void *, size_t);
int            strcmp      (const char *, const char *);
int            strncmp     (const char *, const char *, size_t);
char           *strcpy     (char *, const char *);
char           *strncpy    (char *, const char *, size_t);
size_t         strlen      (const char *);
char           *strcat     (char *, const char *);
char           *strchr     (const char *, int);
int            strcasecmp  (const char *, const char *);
int            strncasecmp (const char *, const char *, size_t);
char           *strncpy    (char *, const char *, size_t);
int            strncmp     (const char *, const char *, size_t);
char           *strrchr    (const char *, int);
unsigned long  strtoul     (const char *, char **, int);
long           strtol      (const char *, char **, int);
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
void           qsort       (void *, size_t, size_t, int (*)(const void *, const void *));
char           *getenv     (const char *);
void           exit        (int);
void           abort       (void);
__sighandler_t *signal     (int, __sighandler_t *);

//
// Global variables from EFI Application Toolkit required to buiild Open SSL
//
extern FILE  *stderr;
extern FILE  *stdin;
extern FILE  *stdout;

#define AsciiStrLen(x) strlena(x)
#define AsciiStrnCmp(s1, s2, len) strncmpa(s1, s2, len)

//
// Macros that directly map functions to BaseLib, BaseMemoryLib, and DebugLib functions
//
#define memcpy(dest,source,count)         ( {CopyMem(dest,source,(UINTN)(count)); dest; })
#define memset(dest,ch,count)             SetMem(dest,(UINTN)(count),(UINT8)(ch))
#define memchr(buf,ch,count)              ScanMem8(buf,(UINTN)(count),(UINT8)ch)
#define memcmp(buf1,buf2,count)           (int)(CompareMem(buf1,buf2,(UINTN)(count)))
#define memmove(dest,source,count)        CopyMem(dest,source,(UINTN)(count))
#define strcmp                            strcmpa
#define strncmp(string1,string2,count)    (int)(AsciiStrnCmp(string1,string2,(UINTN)(count)))
#define strcpy(strDest,strSource)         AsciiStrCpy(strDest,strSource)
#define strncpy(strDest,strSource,count)  AsciiStrnCpy(strDest,strSource,(UINTN)count)
#define strlen(str)                       (size_t)(AsciiStrLen(str))
#define strcat(strDest,strSource)         AsciiStrCat(strDest,strSource)
#define strchr(str,ch)                    ScanMem8((VOID *)(str),AsciiStrSize(str),(UINT8)ch)
#define abort()                           ASSERT (FALSE)
#define assert(expression)
#define localtime(timer)                  NULL
#define gmtime_r(timer,result)            (result = NULL)
#define atoi(nptr)                        Atoi(nptr)

#endif
