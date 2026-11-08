// Compat Win32 -> Android/Linux (clang/gcc). Forcado em todos os arquivos via "-include" no CMake.
#ifndef AZ_ANDROID_COMPAT_H
#define AZ_ANDROID_COMPAT_H

#ifndef _WIN32

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#ifndef _cdecl
#define _cdecl
#endif
#ifndef __cdecl
#define __cdecl
#endif
#ifndef _stdcall
#define _stdcall
#endif
#ifndef __stdcall
#define __stdcall
#endif
#ifndef __declspec
#define __declspec(x)
#endif
#ifndef _MAX_PATH
#define _MAX_PATH 260
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#define stricmp   strcasecmp
#define _stricmp  strcasecmp
#define strnicmp  strncasecmp
#define _strnicmp strncasecmp
#define _snprintf snprintf
#define _vsnprintf vsnprintf
// clang trata _rotr como builtin (MSVC); util.cpp define a propria versao.
#define _rotr az_rotr
static inline char* strupr(char* s) { for (char* p = s; *p; ++p) *p = (char)toupper((unsigned char)*p); return s; }
static inline char* strlwr(char* s) { for (char* p = s; *p; ++p) *p = (char)tolower((unsigned char)*p); return s; }
#define _strdup   strdup
#define _unlink   unlink
#define _getcwd   getcwd
#define _isnan    isnan
static inline char* itoa(int value, char* str, int base)
{
	if (base == 16) snprintf(str, 34, "%x", (unsigned)value);
	else if (base == 8) snprintf(str, 34, "%o", (unsigned)value);
	else snprintf(str, 34, "%d", value);
	return str;
}
#define _finite   finite_compat
static inline int finite_compat(double x) { return isfinite(x); }

#ifdef __cplusplus
#include <algorithm>
#include <type_traits>
template <class A, class B> static inline typename std::common_type<A,B>::type min(A a, B b) { return (a < b) ? a : b; }
template <class A, class B> static inline typename std::common_type<A,B>::type max(A a, B b) { return (a > b) ? a : b; }
#else
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#endif

// Em Android (clang) alguns SDKs antigos usam estes nomes.
typedef unsigned char BOOLEAN;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#endif // __ANDROID__ || __linux__
#endif
