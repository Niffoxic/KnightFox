#pragma once

// Detect MSVC + SAL
#if defined(_MSC_VER)
#include <sal.h>
#else

#ifndef _In_
#   define _In_
#endif
#ifndef _Out_
#   define _Out_
#endif
#ifndef _Inout_
#   define _Inout_
#endif

#ifndef _In_opt_
#   define _In_opt_
#endif
#ifndef _Out_opt_
#   define _Out_opt_
#endif
#ifndef _Inout_opt_
#   define _Inout_opt_
#endif

#ifndef _In_reads_
#   define _In_reads_(x)
#endif
#ifndef _In_reads_opt_
#   define _In_reads_opt_(x)
#endif
#ifndef _Out_writes_
#   define _Out_writes_(x)
#endif
#ifndef _Out_writes_opt_
#   define _Out_writes_opt_(x)
#endif
#ifndef _Inout_updates_
#   define _Inout_updates_(x)
#endif

#ifndef _In_reads_bytes_
#   define _In_reads_bytes_(x)
#endif
#ifndef _In_reads_bytes_opt_
#   define _In_reads_bytes_opt_(x)
#endif
#ifndef _Out_writes_bytes_
#   define _Out_writes_bytes_(x)
#endif
#ifndef _Out_writes_bytes_opt_
#   define _Out_writes_bytes_opt_(x)
#endif
#ifndef _Inout_updates_bytes_
#   define _Inout_updates_bytes_(x)
#endif

#ifndef _In_z_
#   define _In_z_
#endif
#ifndef _In_opt_z_
#   define _In_opt_z_
#endif
#ifndef _Out_writes_z_
#   define _Out_writes_z_(x)
#endif
#ifndef _Out_writes_opt_z_
#   define _Out_writes_opt_z_(x)
#endif

#ifndef _Ret_notnull_
#   define _Ret_notnull_
#endif
#ifndef _Ret_maybenull_
#   define _Ret_maybenull_
#endif
#ifndef _Ret_z_
#   define _Ret_z_
#endif

#ifndef _Use_decl_annotations_
#   define _Use_decl_annotations_
#endif

#ifndef _Check_return_
#   define _Check_return_
#endif
#ifndef _Success_
#   define _Success_(expr)
#endif

#ifndef _COM_Outptr_
#   define _COM_Outptr_
#endif
#ifndef _COM_Outptr_opt_
#   define _COM_Outptr_opt_
#endif
#ifndef _Outptr_result_maybenull_
#   define _Outptr_result_maybenull_
#endif
#ifndef _Outptr_result_nullonfailure_
#   define _Outptr_result_nullonfailure_
#endif
#endif // _MSC_VER

#ifndef NODISCARD
#define NODISCARD [[nodiscard]] _Check_return_
#endif

// For HRESULT-like APIs where success/failure matters
#ifndef KFE_CHECK_RETURN
#define KFE_CHECK_RETURN _Check_return_
#endif

// For functions that indicate success/failure via bool
#ifndef KFE_SUCCESS
#define KFE_SUCCESS(expr) _Success_(expr)
#endif

//  Parameter direction

// Basic in/out
#define KFE_IN          _In_
#define KFE_OUT         _Out_
#define KFE_INOUT       _Inout_

#define KFE_IN_CONST    _In_

#define KFE_IN_OPT      _In_opt_
#define KFE_OUT_OPT     _Out_opt_
#define KFE_INOUT_OPT   _Inout_opt_

//  Buffers (element counts)

// Read-only input buffers (e.g. vertices, indices)
#define KFE_IN_READS(count)            _In_reads_(count)
#define KFE_IN_READS_OPT(count)        _In_reads_opt_(count)

// Output buffers (filled by the function)
#define KFE_OUT_WRITES(count)          _Out_writes_(count)
#define KFE_OUT_WRITES_OPT(count)      _Out_writes_opt_(count)

// In/out buffers (updated in-place)
#define KFE_INOUT_UPDATES(count)       _Inout_updates_(count)

//  Buffers (byte counts)
#define KFE_IN_READS_BYTES(bytes)          _In_reads_bytes_(bytes)
#define KFE_IN_READS_BYTES_OPT(bytes)      _In_reads_bytes_opt_(bytes)

#define KFE_OUT_WRITES_BYTES(bytes)        _Out_writes_bytes_(bytes)
#define KFE_OUT_WRITES_BYTES_OPT(bytes)    _Out_writes_bytes_opt_(bytes)

#define KFE_INOUT_UPDATES_BYTES(bytes)     _Inout_updates_bytes_(bytes)

// Input zero-terminated string
#define KFE_IN_STR              _In_z_
#define KFE_IN_STR_OPT          _In_opt_z_

// Output: writes 'count' chars and ensures zero-termination
#define KFE_OUT_STR(count)      _Out_writes_z_(count)
#define KFE_OUT_STR_OPT(count)  _Out_writes_opt_z_(count)

// Return value is a zero-terminated string
#define KFE_RET_STR             _Ret_z_

// Return: pointer will NOT be null on success
#define KFE_RET_NOT_NULL        _Ret_notnull_

// Return: pointer may be null
#define KFE_RET_MAYBE_NULL      _Ret_maybenull_

// For "out" pointers that receive allocated objects
#define KFE_COM_OUT             _COM_Outptr_
#define KFE_COM_OUT_OPT         _COM_Outptr_opt_

// Outptr that might be null even on success
#define KFE_OUTPTR_MAYBE_NULL   _Outptr_result_maybenull_

// On failure, the outptr will be set to null
#define KFE_OUTPTR_NULL_ON_FAIL _Outptr_result_nullonfailure_

#define KFE_USE_DECL_ANNOTS     _Use_decl_annotations_

// Example: HRESULT MyFunc(_In_ const wchar_t* path, _COM_Outptr_ IThing** obj);
#define KFE_IN_CWSTR            KFE_IN_STR const wchar_t*
#define KFE_IN_CSTR             KFE_IN_STR const char*
#define KFE_OUT_COM_PTR(T)      KFE_COM_OUT T**
#define KFE_OUT_COM_PTR_OPT(T)  KFE_COM_OUT_OPT T**
