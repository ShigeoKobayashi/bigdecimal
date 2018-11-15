#ifdef _WIN32
#pragma once
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN 
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <limits>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>

#include "bigdecimal.h"

#ifdef _DEBUG
#define ASSERT(f)    DbgAssert(f,__FILE__,__LINE__)
void    DbgAssert(int  f,char *file,int line);
#define TRACE(s) printf(s)
#else 
#define ASSERT(f)
#define TRACE(s)
#endif // _DEBUG

/*
 *  ------------------
 *  MACRO definitions.
 *  ------------------
 */
#define Abs(a)        (((a)>= 0)?(a):(-(a)))
#define Max(a, b)     (((a)>(b))?(a):(b))
#define Min(a, b)     (((a)>(b))?(b):(a))
#define MemCmp(x,y,z) memcmp(x,y,z)
#define StrCmp(x,y)   strcmp(x,y)
#define VpGetFlag(a)  ((a)->flag)


// Alloc/Free macros
#define VP_FREE_REAL(p)     VpFree((VP_HANDLE*)(&p))
#define VP_SIGN(p)          ((Real*)p)->sign
#define VP_FINITE_SIGN(p,s) VP_SIGN(p) = (s)>0?VP_SIGN_POSITIVE_FINITE:VP_SIGN_NEGATIVE_FINITE
#define VP_EXPONENT(a)      (a->exponent)
