//----------------------------------------------------------------------------------------------------------------------
// Types.h
//
// Copyright (C) 2010 WhiteMoon Dreams, Inc.
// All Rights Reserved
//----------------------------------------------------------------------------------------------------------------------

#pragma once
#ifndef LUNAR_CORE_TYPES_H
#define LUNAR_CORE_TYPES_H

#include "Core/Char.h"

#ifndef _MSC_VER

// Use inttypes.h where available; we simply try to provide relevant type definitions for platforms that don't provide
// it.
#include <inttypes.h>

#endif

#ifdef _MSC_VER

/// @defgroup inttypes Integer Types
/// We use the integer types defined in stdint.h on platforms where it is available.  If it is not available, then we
/// define compatible types here.
//@{

/// 8-bit signed integer.
typedef signed char int8_t;
/// 16-bit signed integer.
typedef signed short int16_t;
/// 32-bit signed integer.
typedef signed int int32_t;
/// 64-bit signed integer.
typedef signed __int64 int64_t;

/// 8-bit unsigned integer.
typedef unsigned char uint8_t;
/// 16-bit unsigned integer.
typedef unsigned short uint16_t;
/// 32-bit unsigned integer.
typedef unsigned int uint32_t;
/// 64-bit unsigned integer.
typedef unsigned __int64 uint64_t;

/// Fastest signed integer of at least 8 bits.
typedef int8_t int_fast8_t;
/// Fastest signed integer of at least 16 bits.
typedef int32_t int_fast16_t;
/// Fastest signed integer of at least 32 bits.
typedef int32_t int_fast32_t;
/// Fastest signed integer of at least 64 bits.
typedef int64_t int_fast64_t;

/// Fastest unsigned integer of at least 8 bits.
typedef uint8_t uint_fast8_t;
/// Fastest unsigned integer of at least 16 bits.
typedef uint32_t uint_fast16_t;
/// Fastest unsigned integer of at least 32 bits.
typedef uint32_t uint_fast32_t;
/// Fastest unsigned integer of at least 64 bits.
typedef uint64_t uint_fast64_t;

//@}

/// @defgroup intlimits Integer Limits
//@{

/// Minimum signed 8-bit integer.
#define INT8_MIN ( -128 )
/// Minimum signed 16-bit integer.
#define INT16_MIN ( -32768 )
/// Minimum signed 32-bit integer.
#define INT32_MIN ( -2147483647 - 1 )
/// Minimum signed 64-bit integer.
#define INT64_MIN ( -9223372036854775807i64 - 1i64 )

/// Maximum signed 8-bit integer.
#define INT8_MAX ( 127 )
/// Maximum signed 16-bit integer.
#define INT16_MAX ( 32767 )
/// Maximum signed 32-bit integer.
#define INT32_MAX ( 2147483647 )
/// Maximum signed 64-bit integer.
#define INT64_MAX ( 9223372036854775807i64 )

/// Maximum unsigned 8-bit integer.
#define UINT8_MAX ( 255 )
/// Maximum unsigned 16-bit integer.
#define UINT16_MAX ( 65535 )
/// Maximum unsigned 32-bit integer.
#define UINT32_MAX ( 4294967295UL )
/// Maximum unsigned 64-bit integer.
#define UINT64_MAX ( 18446744073709551615ui64 )

/// Minimum int_fast8_t value.
#define INT_FAST8_MIN INT8_MIN
/// Minimum int_fast16_t value.
#define INT_FAST16_MIN INT32_MIN
/// Minimum int_fast32_t value.
#define INT_FAST32_MIN INT32_MIN
/// Minimum int_fast64_t value.
#define INT_FAST64_MIN INT64_MIN

/// Maximum int_fast8_t value.
#define INT_FAST8_MAX INT8_MAX
/// Maximum int_fast16_t value.
#define INT_FAST16_MAX INT32_MAX
/// Maximum int_fast32_t value.
#define INT_FAST32_MAX INT32_MAX
/// Maximum int_fast64_t value.
#define INT_FAST64_MAX INT64_MAX

/// Maximum uint_fast8_t value.
#define UINT_FAST8_MAX UINT8_MAX
/// Maximum uint_fast16_t value.
#define UINT_FAST16_MAX UINT32_MAX
/// Maximum uint_fast32_t value.
#define UINT_FAST32_MAX UINT32_MAX
/// Maximum uint_fast64_t value.
#define UINT_FAST64_MAX UINT64_MAX

//@}

/// @defgroup intprintf Integer "printf" Formatting Macros
/// These allow portable usage of fixed-sized integers in formatting strings for printf() and similar statements.
/// Macros defined in inttypes.h are used when available on supported platforms.
//@{

/// "char" string format macro for signed 8-bit integers.
#define PRId8 "hhd"
/// "char" string format macro for signed 16-bit integers.
#define PRId16 "hd"
/// "char" string format macro for signed 32-bit integers.
#define PRId32 "I32d"
/// "char" string format macro for signed 64-bit integers.
#define PRId64 "I64d"

/// "char" string format macro for unsigned 8-bit integers.
#define PRIu8 "hhu"
/// "char" string format macro for unsigned 16-bit integers.
#define PRIu16 "hu"
/// "char" string format macro for unsigned 32-bit integers.
#define PRIu32 "I32u"
/// "char" string format macro for unsigned 64-bit integers.
#define PRIu64 "I64u"

/// "char" string format macro for int_fast8_t.
#define PRIdFAST8 PRId8
/// "char" string format macro for int_fast16_t.
#define PRIdFAST16 PRId32
/// "char" string format macro for int_fast32_t.
#define PRIdFAST32 PRId32
/// "char" string format macro for int_fast64_t.
#define PRIdFAST64 PRId64

/// "char" string format macro for uint_fast8_t.
#define PRIuFAST8 PRIu8
/// "char" string format macro for uint_fast16_t.
#define PRIuFAST16 PRIu32
/// "char" string format macro for uint_fast32_t.
#define PRIuFAST32 PRIu32
/// "char" string format macro for uint_fast64_t.
#define PRIuFAST64 PRIu64

/// "char" string format macro for ptrdiff_t.
#define PRIdPD "Id"
/// "char" string format macro for size_t.
#define PRIuSZ "Iu"

/// "wchar_t" string format macro for signed 8-bit integers.
#define WPRId8 L"hhd"
/// "wchar_t" string format macro for signed 16-bit integers.
#define WPRId16 L"hd"
/// "wchar_t" string format macro for signed 32-bit integers.
#define WPRId32 L"I32d"
/// "wchar_t" string format macro for signed 64-bit integers.
#define WPRId64 L"I64d"

/// "wchar_t" string format macro for unsigned 8-bit integers.
#define WPRIu8 L"hhu"
/// "wchar_t" string format macro for unsigned 16-bit integers.
#define WPRIu16 L"hu"
/// "wchar_t" string format macro for unsigned 32-bit integers.
#define WPRIu32 L"I32u"
/// "wchar_t" string format macro for unsigned 64-bit integers.
#define WPRIu64 L"I64u"

/// "wchar_t" string format macro for int_fast8_t.
#define WPRIdFAST8 WPRId8
/// "wchar_t" string format macro for int_fast16_t.
#define WPRIdFAST16 WPRId32
/// "wchar_t" string format macro for int_fast32_t.
#define WPRIdFAST32 WPRId32
/// "wchar_t" string format macro for int_fast64_t.
#define WPRIdFAST64 WPRId64

/// "wchar_t" string format macro for uint_fast8_t.
#define WPRIuFAST8 WPRIu8
/// "wchar_t" string format macro for uint_fast16_t.
#define WPRIuFAST16 WPRIu32
/// "wchar_t" string format macro for uint_fast32_t.
#define WPRIuFAST32 WPRIu32
/// "wchar_t" string format macro for uint_fast64_t.
#define WPRIuFAST64 WPRIu64

/// "wchar_t" string format macro for ptrdiff_t.
#define WPRIdPD L"Id"
/// "wchar_t" string format macro for size_t.
#define WPRIuSZ L"Iu"

/// "tchar_t" string format macro for signed 8-bit integers.
#define TPRId8 L_T( "hhd" )
/// "tchar_t" string format macro for signed 16-bit integers.
#define TPRId16 L_T( "hd" )
/// "tchar_t" string format macro for signed 32-bit integers.
#define TPRId32 L_T( "I32d" )
/// "tchar_t" string format macro for signed 64-bit integers.
#define TPRId64 L_T( "I64d" )

/// "tchar_t" string format macro for unsigned 8-bit integers.
#define TPRIu8 L_T( "hhu" )
/// "tchar_t" string format macro for unsigned 16-bit integers.
#define TPRIu16 L_T( "hu" )
/// "tchar_t" string format macro for unsigned 32-bit integers.
#define TPRIu32 L_T( "I32u" )
/// "tchar_t" string format macro for unsigned 64-bit integers.
#define TPRIu64 L_T( "I64u" )

/// "tchar_t" string format macro for int_fast8_t.
#define TPRIdFAST8 TPRId8
/// "tchar_t" string format macro for int_fast16_t.
#define TPRIdFAST16 TPRId32
/// "tchar_t" string format macro for int_fast32_t.
#define TPRIdFAST32 TPRId32
/// "tchar_t" string format macro for int_fast64_t.
#define TPRIdFAST64 TPRId64

/// "tchar_t" string format macro for uint_fast8_t.
#define TPRIuFAST8 TPRIu8
/// "tchar_t" string format macro for uint_fast16_t.
#define TPRIuFAST16 TPRIu32
/// "tchar_t" string format macro for uint_fast32_t.
#define TPRIuFAST32 TPRIu32
/// "tchar_t" string format macro for uint_fast64_t.
#define TPRIuFAST64 TPRIu64

/// "tchar_t" string format macro for ptrdiff_t.
#define TPRIdPD L_T( "Id" )
/// "tchar_t" string format macro for size_t.
#define TPRIuSZ L_T( "Iu" )

//@}

/// @defgroup intscanf Integer "scanf" Formatting Macros
/// These allow portable usage of fixed-sized integers in formatting strings for scanf() and similar statements.  Macros
/// defined in inttypes.h are used instead on supported platforms.
//@{

/// "char" string format macro for signed 8-bit integers.
#define SCNd8 "hhd"
/// "char" string format macro for signed 16-bit integers.
#define SCNd16 "hd"
/// "char" string format macro for signed 32-bit integers.
#define SCNd32 "I32d"
/// "char" string format macro for signed 64-bit integers.
#define SCNd64 "I64d"

/// "char" string format macro for unsigned 8-bit integers.
#define SCNu8 "hhu"
/// "char" string format macro for unsigned 16-bit integers.
#define SCNu16 "hu"
/// "char" string format macro for unsigned 32-bit integers.
#define SCNu32 "I32u"
/// "char" string format macro for unsigned 64-bit integers.
#define SCNu64 "I64u"

/// "char" string format macro for int_fast8_t.
#define SCNdFAST8 SCNd8
/// "char" string format macro for int_fast16_t.
#define SCNdFAST16 SCNd32
/// "char" string format macro for int_fast32_t.
#define SCNdFAST32 SCNd32
/// "char" string format macro for int_fast64_t.
#define SCNdFAST64 SCNd64

/// "char" string format macro for uint_fast8_t.
#define SCNuFAST8 SCNu8
/// "char" string format macro for uint_fast16_t.
#define SCNuFAST16 SCNu32
/// "char" string format macro for uint_fast32_t.
#define SCNuFAST32 SCNu32
/// "char" string format macro for uint_fast64_t.
#define SCNuFAST64 SCNu64

/// "char" string format macro for ptrdiff_t.
#define SCNdPD "Id"
/// "char" string format macro for size_t.
#define SCNuSZ "Iu"

/// "wchar_t" string format macro for signed 8-bit integers.
#define WSCNd8 L"hhd"
/// "wchar_t" string format macro for signed 16-bit integers.
#define WSCNd16 L"hd"
/// "wchar_t" string format macro for signed 32-bit integers.
#define WSCNd32 L"I32d"
/// "wchar_t" string format macro for signed 64-bit integers.
#define WSCNd64 L"I64d"

/// "wchar_t" string format macro for unsigned 8-bit integers.
#define WSCNu8 L"hhu"
/// "wchar_t" string format macro for unsigned 16-bit integers.
#define WSCNu16 L"hu"
/// "wchar_t" string format macro for unsigned 32-bit integers.
#define WSCNu32 L"I32u"
/// "wchar_t" string format macro for unsigned 64-bit integers.
#define WSCNu64 L"I64u"

/// "wchar_t" string format macro for int_fast8_t.
#define WSCNdFAST8 WSCNd8
/// "wchar_t" string format macro for int_fast16_t.
#define WSCNdFAST16 WSCNd32
/// "wchar_t" string format macro for int_fast32_t.
#define WSCNdFAST32 WSCNd32
/// "wchar_t" string format macro for int_fast64_t.
#define WSCNdFAST64 WSCNd64

/// "wchar_t" string format macro for uint_fast8_t.
#define WSCNuFAST8 WSCNu8
/// "wchar_t" string format macro for uint_fast16_t.
#define WSCNuFAST16 WSCNu32
/// "wchar_t" string format macro for uint_fast32_t.
#define WSCNuFAST32 WSCNu32
/// "wchar_t" string format macro for uint_fast64_t.
#define WSCNuFAST64 WSCNu64

/// "wchar_t" string format macro for ptrdiff_t.
#define WSCNdPD L"Id"
/// "wchar_t" string format macro for size_t.
#define WSCNuSZ L"Iu"

/// "tchar_t" string format macro for signed 8-bit integers.
#define TSCNd8 L_T( "hhd" )
/// "tchar_t" string format macro for signed 16-bit integers.
#define TSCNd16 L_T( "hd" )
/// "tchar_t" string format macro for signed 32-bit integers.
#define TSCNd32 L_T( "I32d" )
/// "tchar_t" string format macro for signed 64-bit integers.
#define TSCNd64 L_T( "I64d" )

/// "tchar_t" string format macro for unsigned 8-bit integers.
#define TSCNu8 L_T( "hhu" )
/// "tchar_t" string format macro for unsigned 16-bit integers.
#define TSCNu16 L_T( "hu" )
/// "tchar_t" string format macro for unsigned 32-bit integers.
#define TSCNu32 L_T( "I32u" )
/// "tchar_t" string format macro for unsigned 64-bit integers.
#define TSCNu64 L_T( "I64u" )

/// "tchar_t" string format macro for int_fast8_t.
#define TSCNdFAST8 TSCNd8
/// "tchar_t" string format macro for int_fast16_t.
#define TSCNdFAST16 TSCNd32
/// "tchar_t" string format macro for int_fast32_t.
#define TSCNdFAST32 TSCNd32
/// "tchar_t" string format macro for int_fast64_t.
#define TSCNdFAST64 TSCNd64

/// "tchar_t" string format macro for uint_fast8_t.
#define TSCNuFAST8 TSCNu8
/// "tchar_t" string format macro for uint_fast16_t.
#define TSCNuFAST16 TSCNu32
/// "tchar_t" string format macro for uint_fast32_t.
#define TSCNuFAST32 TSCNu32
/// "tchar_t" string format macro for uint_fast64_t.
#define TSCNuFAST64 TSCNu64

/// "tchar_t" string format macro for ptrdiff_t.
#define TSCNdPD L_T( "Id" )
/// "tchar_t" string format macro for size_t.
#define TSCNuSZ L_T( "Iu" )

//@}

#endif  // #ifdef _MSC_VER

/// @defgroup floattypes Floating-point Types
/// While these may not be particularly necessary, they do provide some level of consistency with the integer types.
//@{

/// Single-precision floating-point.
typedef float float32_t;
/// Double-precision floating-point.
typedef double float64_t;

//@}

#endif  // LUNAR_CORE_TYPES_H