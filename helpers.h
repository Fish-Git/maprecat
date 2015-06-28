/*********************************************************************/
/*                    Helper functions/macros                        */
/*********************************************************************/

#pragma once

/*********************************************************************/

#ifdef      _MSC_VER    // windows?
    #define _MSVC_      // windows!
    #define _CRT_SECURE_NO_WARNINGS  1
#endif

/*********************************************************************/

#ifdef _MSVC_

    #include <windows.h>

    #include <stdlib.h>
    #include <stdio.h>
    #include <string.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <sys/stat.h>

    #include <io.h>
    #include <conio.h>
    #include <share.h>

#else // non-windows

    #include <unistd.h>

    #include <stdlib.h>
    #include <stdio.h>
    #include <string.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <sys/stat.h>

    #include <inttypes.h>
    #include <sys/types.h>

    #define HAVE_INTTYPES_H
    #define HAVE_U_INT8_T
    #define HAVE_INTTYPES_H

#endif // _MSVC_

/*********************************************************************/

#ifdef _MSVC_

    typedef unsigned char       u_char;
    typedef unsigned int        u_int;
    typedef unsigned long       u_long;

    typedef unsigned __int8     u_int8_t;
    typedef   signed __int8       int8_t;

    typedef unsigned __int16    u_int16_t;
    typedef   signed __int16      int16_t;

    typedef unsigned __int32    u_int32_t;
    typedef   signed __int32      int32_t;

    typedef unsigned __int64    u_int64_t;
    typedef   signed __int64      int64_t;

    #define  HAVE_U_INT
    #define  HAVE_U_INT8_T

    #define  inline             __inline
    #define  __inline__         __inline

#endif // _MSVC_

/*********************************************************************/

#ifndef INLINE
    #ifdef _MSVC_
        #define INLINE          __inline
    #else
        #define INLINE          inline
    #endif
#endif

#ifndef     TRUE
    #define TRUE                1
#endif

#ifndef     FALSE
    #define FALSE               0
#endif

#ifndef __cplusplus
    #define true                1
    #define false               0
#endif

/*********************************************************************/

#ifndef HAVE_INTTYPES_H
  #ifdef HAVE_U_INT
    #define  uint8_t   u_int8_t
    #define  uint16_t  u_int16_t
    #define  uint32_t  u_int32_t
    #define  uint64_t  u_int64_t
  #else
    #error Unable to find fixed-size data types
  #endif
#endif

#ifndef HAVE_U_INT8_T
  #ifdef HAVE_INTTYPES_H
    typedef  uint8_t   u_int8_t;
    typedef  uint16_t  u_int16_t;
    typedef  uint32_t  u_int32_t;
    typedef  uint64_t  u_int64_t;
  #else
    #error Unable to define u_intNN_t data types
  #endif
#endif

typedef  int8_t     S8;         // signed 8-bits
typedef  int16_t    S16;        // signed 16-bits
typedef  int32_t    S32;        // signed 32-bits
typedef  int64_t    S64;        // signed 64-bits

typedef  uint8_t    U8;         // unsigned 8-bits
typedef  uint16_t   U16;        // unsigned 16-bits
typedef  uint32_t   U32;        // unsigned 32-bits
typedef  uint64_t   U64;        // unsigned 64-bits

/*********************************************************************/

#ifdef _MSVC_

  static __inline  uint16_t  __fastcall  bswap_16 ( uint16_t  x )
  {
    return _byteswap_ushort((x));
  }

  static __inline  uint32_t  __fastcall  bswap_32 ( uint32_t  x )
  {
    return _byteswap_ulong((x));
  }

  static __inline  uint64_t  __fastcall  bswap_64 ( uint64_t  x )
  {
    return _byteswap_uint64((x));
  }

#else // non-windows

  #define bswap_16(x) \
          ( (((x) & 0xFF00) >> 8) \
          | (((x) & 0x00FF) << 8) )

  #define bswap_32(x) \
          ( (((x) & 0xFF000000) >> 24) \
          | (((x) & 0x00FF0000) >> 8)  \
          | (((x) & 0x0000FF00) << 8)  \
          | (((x) & 0x000000FF) << 24) )

  #define bswap_64(x) \
        ( ((U64)((x) & 0xFF00000000000000ULL) >> 56) \
        | ((U64)((x) & 0x00FF000000000000ULL) >> 40) \
        | ((U64)((x) & 0x0000FF0000000000ULL) >> 24) \
        | ((U64)((x) & 0x000000FF00000000ULL) >> 8)  \
        | ((U64)((x) & 0x00000000FF000000ULL) << 8)  \
        | ((U64)((x) & 0x0000000000FF0000ULL) << 24) \
        | ((U64)((x) & 0x000000000000FF00ULL) << 40) \
        | ((U64)((x) & 0x00000000000000FFULL) << 56) )

#endif // _MSVC_

#ifdef WORDS_BIGENDIAN
  #define CSWAP16(x)    (x)
  #define CSWAP32(x)    (x)
  #define CSWAP64(x)    (x)
#else
  #define CSWAP16(x)    bswap_16(x)
  #define CSWAP32(x)    bswap_32(x)
  #define CSWAP64(x)    bswap_64(x)
#endif

#define FETCH_HW( var, ptr )    (var)  =  fetch_hw( ptr )
#define FETCH_FW( var, ptr )    (var)  =  fetch_fw( ptr )
#define FETCH_DW( var, ptr )    (var)  =  fetch_dw( ptr )


/*-------------------------------------------------------------------
 * fetch_hw_noswap and fetch_hw
 *-------------------------------------------------------------------*/
#if !defined(fetch_hw_noswap)
  #if defined(fetch_hw)
    #define fetch_hw_noswap(_p) CSWAP16(fetch_hw((_p)))
  #else
    #if !defined(OPTION_STRICT_ALIGNMENT)
      static __inline__ U16 fetch_hw_noswap(void *ptr) {
        return *(U16 *)ptr;
      }
    #else
      static __inline__ U16 fetch_hw_noswap(void *ptr) {
        U16 value;
        memcpy(&value, (BYTE *)ptr, 2);
        return value;
      }
    #endif
  #endif
#endif
#if !defined(fetch_hw)
  #define fetch_hw(_p) CSWAP16(fetch_hw_noswap((_p)))
#endif


/*-------------------------------------------------------------------
 * fetch_fw_noswap and fetch_fw
 *-------------------------------------------------------------------*/
#if !defined(fetch_fw_noswap)
  #if defined(fetch_fw)
    #define fetch_fw_noswap(_p) CSWAP32(fetch_fw((_p)))
  #else
    #if !defined(OPTION_STRICT_ALIGNMENT)
      static __inline__ U32 fetch_fw_noswap(const void *ptr) {
        return *(U32 *)ptr;
      }
    #else
      static __inline__ U32 fetch_fw_noswap(const void *ptr) {
        U32 value;
        memcpy(&value, (BYTE *)ptr, 4);
        return value;
      }
    #endif
  #endif
#endif
#if !defined(fetch_fw)
  #define fetch_fw(_p) CSWAP32(fetch_fw_noswap((_p)))
#endif


/*-------------------------------------------------------------------
 * fetch_dw_noswap and fetch_dw
 *-------------------------------------------------------------------*/
#if !defined(fetch_dw_noswap)
  #if defined(fetch_dw)
    #define fetch_dw_noswap(_p) CSWAP64(fetch_dw((_p)))
  #else
    #if !defined(OPTION_STRICT_ALIGNMENT)
      static __inline__ U64 fetch_dw_noswap(void *ptr) {
        return *(U64 *)ptr;
      }
    #else
      static __inline__ U64 fetch_dw_noswap(void *ptr) {
        U64 value;
        memcpy(&value, (BYTE *)ptr, 8);
        return value;
      }
    #endif
  #endif
#endif
#if !defined(fetch_dw)
  #define fetch_dw(_p) CSWAP64(fetch_dw_noswap((_p)))
#endif

/*********************************************************************/

#ifdef _MSVC_

    #define  HOPEN      w32_hopen
    #define  close      _close
    #define  read       _read
    #define  write      _write
    #define  lseek      _lseeki64

    #define  S_IRUSR    _S_IREAD
    #define  S_IWUSR    _S_IWRITE
    #define  S_IRGRP    _S_IREAD

    #define PAUSEIFBEINGDEBUGGED()                  \
    do                                              \
    {                                               \
        if (IsDebuggerPresent())                    \
        {                                           \
            printf( "Press any key to exit..." );   \
            while (!_kbhit()) Sleep(50);            \
        }                                           \
    }                                               \
    while (0)

#else // non-windows

    #define  HOPEN      open
    #define  O_BINARY   0

    #define PAUSEIFBEINGDEBUGGED()

#endif // _MSVC_


/*********************************************************************/

#ifdef _MSVC_
static INLINE
int w32_hopen( const char* path, int oflag, ... )
{
    int pmode   = _S_IREAD | _S_IWRITE;
    int sh_flg  = _SH_DENYWR;
    int fd      = -1;

    if (oflag & O_CREAT)
    {
        va_list vargs;
        va_start( vargs, oflag );
        pmode = va_arg( vargs, int );
    }

    _sopen_s( &fd, path, oflag, sh_flg, pmode );

    return fd;
}
#endif // _MSVC_

/*********************************************************************/
