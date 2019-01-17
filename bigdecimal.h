/*
 *
 * BigDecimal: Variable precision decimal arithmetic C/C++ library.
 *
 * Copyright(C) 2012 by Shigeo Kobayashi(shigeo@tinyforest.jp)
 *
 *     Version 1.1 ... VpLoad() added.
 *
 */

#ifndef  _BIG_DECIMAL_H
#define  _BIG_DECIMAL_H 1

#ifdef _WIN32
#ifndef WINDOWS
    #define WINDOWS
    #ifdef _WIN64
        /* 64bit Windows */
    #define BIT64
    #else
        /* 32bit Windows */
    #define BIT32
    #endif
#endif
#else
#ifndef LINUX
    #define LINUX
    #ifdef __x86_64__
            /* 64bit Linux */
        #define BIT64
       #else
           /* 32bit Linux */
        #define BIT32
    #endif
#endif
#endif

#ifdef WINDOWS /**** WINDOWS ****/
/* WINDOWS specific part (Same define's must be defined for other platforms.) */
 #define VP_EXPORT(t)  __declspec(dllexport) t __stdcall
/* Note:
__stdcall __cdecl and C#
------------------------
  WIN32 API's use __stdcall(which can't be used for vararg functions) with .def file representation.
  __stdcall without .def file changes function name exported in .lib file.
  __cdecl (c compiler default) never changes function name exported but consumes more memories than __stdcall.
  C# [Dllexport] atrribute uses __stdcall in default.
  To call __cdecl functions from C#, use CallingConvention.Cdecl like "[DllImport("MyDLL.dll", CallingConvention = CallingConvention.Cdecl)]".
*/
#endif /**** WINDOWS ****/

#ifdef LINUX /******** LINUX ********/
/* gcc/g++ specific part ==> compiled with '-fPIC -fvisibility=hidden' option. */
/*
 -fvisibility=hidden option hides everything except explicitly declared as exported API just like
 as Windows' dll.
*/
#define VP_EXPORT(t) __attribute__((visibility ("default")))  t
#endif /**** LINUX ****/

#if defined(__cplusplus)
extern "C" {
#endif

/***/
/* VP_HANDLE,VP_DIGIT,VP_UINT must be unsigned. */
/***/
#define VP_HANDLE  unsigned long long  /* This is actually an address for structure Real. */

/* unsigned log is 32-bit on Windows,but it is 64-bit on 64-bit Linux */
#define VP_DIGIT   unsigned long       /* Fraction part array (can be 64-bit:unsigned long long)   */
#define VP_UINT    unsigned int        /* Common unsigned part   */

VP_EXPORT(int) VpVersion(); /* returns Bigdecimal version. */

/* VP-exception handle called from VP-routines if needed(error case). */
typedef void (VP_EXCEPTION_HANDLER)(VP_HANDLE h,const char *pszMsg);
VP_EXPORT(void) VpSetExceptionHandler(VP_EXCEPTION_HANDLER *pf);

VP_EXPORT(VP_UINT) VpGetTag(VP_HANDLE h);
VP_EXPORT(void   ) VpSetTag(VP_HANDLE h,VP_UINT tag);

/*
 * VP representation
 *  r = 0.xxxxxxxxx *BASE**exponent
 */
typedef struct {
    VP_UINT   Size;    /* all byte size of this structure(used in realloc() case).  */
    VP_UINT   MaxPrec; /* Maximum precision size                          */
                       /* This is the actual size of frac[]               */
                       /*(frac[0] to frac[MaxPrec] are available).        */
    VP_UINT   Prec;    /* Current precision size.                         */
                       /* This indicates how much the.                    */
                       /* the array frac[] is actually used.              */
    VP_UINT   Tag;     /* Space for the user(BigDecimal never touch this) */
    int       exponent;/* Exponent part.                                  */
    int       sign;    /* Attributes of the value.                        */
                       /*
                        *        ==0 : NaN
                        *          1 : Positive zero
                        *         -1 : Negative zero
                        *          2 : Positive number
                        *         -2 : Negative number
                        *          3 : Positive infinite number
                        *         -3 : Negative infinite number
                        */
    VP_DIGIT   frac[1]; /* Array of fraction part. [1] is intensional. */
} Real;

#define VP_SIGN_NaN                0 /* NaN                      */
#define VP_SIGN_POSITIVE_ZERO      1 /* Positive zero            */
#define VP_SIGN_NEGATIVE_ZERO     -1 /* Negative zero            */
#define VP_SIGN_POSITIVE_FINITE    2 /* Positive finite number   */
#define VP_SIGN_NEGATIVE_FINITE   -2 /* Negative finite number   */
#define VP_SIGN_POSITIVE_INFINITE  3 /* Positive infinite number */
#define VP_SIGN_NEGATIVE_INFINITE -3 /* Negative infinite number */

#define VpIsInvalid(h)  (((VP_HANDLE)h)<100)
#define VpIsValid(h)    (((VP_HANDLE)h)>100)
#define VpIsNumeric(h)  ( VpIsValid(h) && !((VpIsNaN(h)||VpIsInf(h)) ))

/* ERROR CODES */
#define VP_ERROR_BAD_STRING         1
#define VP_ERROR_BAD_HANDLE         2
#define VP_ERROR_MEMORY_ALLOCATION  3
#define VP_ERROR_NOT_CONVERGED      4  /* Iteration not converged, */
#define VP_ERROR_NON_NUMERIC        9  /* Bad operation */

/*
 *  NaN & Infinity
 */
#define SZ_NaN  "NaN"
#define SZ_INF  "Infinity"
#define SZ_PINF "+Infinity"
#define SZ_NINF "-Infinity"

/* Round mode */
#define VP_ROUND_UP         1
#define VP_ROUND_DOWN       2
#define VP_ROUND_HALF_UP    3  /* Default mode */
#define VP_ROUND_HALF_DOWN  4
#define VP_ROUND_CEIL       5
#define VP_ROUND_FLOOR      6
#define VP_ROUND_HALF_EVEN  7

VP_EXPORT(VP_HANDLE) VpAlloc(const char *szVal,VP_UINT mx);
VP_EXPORT(VP_HANDLE) VpLoad(VP_HANDLE p,const char *szVal);
VP_EXPORT(VP_HANDLE) VpMemAlloc(VP_UINT mx);
VP_EXPORT(int)       VpAllocCount(); /* returns VP_HANDLE allocation count */
VP_EXPORT(VP_HANDLE) VpClone(VP_HANDLE p);
VP_EXPORT(void)      VpFree(VP_HANDLE *p);
VP_EXPORT(VP_UINT)   VpMaxLength(VP_HANDLE h);
VP_EXPORT(VP_UINT)   VpCurLength(VP_HANDLE h);

VP_EXPORT(VP_UINT)   VpEffectiveDigits(VP_HANDLE h);
VP_EXPORT(int)       VpExponent(VP_HANDLE h);


VP_EXPORT(VP_UINT)   VpGetDigitSeparationCount();          /* default = 10 */
VP_EXPORT(VP_UINT)   VpSetDigitSeparationCount(VP_UINT m);
VP_EXPORT(char)      VpGetDigitSeparator();                /* default = ' ' */
VP_EXPORT(char)      VpSetDigitSeparator(char c);    
VP_EXPORT(char)      VpGetDigitLeader();                   /* default = ' ' */
VP_EXPORT(char)      VpSetDigitLeader(char c);

VP_EXPORT(int)       VpPrintE(FILE *fp, VP_HANDLE h);
VP_EXPORT(int)       VpPrintF(FILE *fp, VP_HANDLE h);
VP_EXPORT(VP_UINT)   VpStringLengthE(VP_HANDLE h);
VP_EXPORT(VP_UINT)   VpStringLengthF(VP_HANDLE h);
VP_EXPORT(char *)    VpToStringE(VP_HANDLE h,char *sz);
VP_EXPORT(char *)    VpToStringF(VP_HANDLE h,char *sz);


/* Sign */
/* Change sign of a to a>0,a<0 if s = 1,-1 respectively */
VP_EXPORT(VP_HANDLE) VpSetSign(VP_HANDLE a,int s);
VP_EXPORT(int)       VpGetSign(VP_HANDLE a);
VP_EXPORT(VP_HANDLE) VpRevertSign(VP_HANDLE a); /* Negate */
#define VpNegate(a)  VpRevertSign(a)
VP_EXPORT(VP_HANDLE) VpAbs(VP_HANDLE c);

/* 1 */
VP_EXPORT(int)       VpIsOne(VP_HANDLE a);
VP_EXPORT(VP_HANDLE) VpSetOne(VP_HANDLE a);

/* ZEROs */
VP_EXPORT(int)       VpIsPosZero(VP_HANDLE a);
VP_EXPORT(int)       VpIsNegZero(VP_HANDLE a);
VP_EXPORT(int)       VpIsZero(VP_HANDLE a);
VP_EXPORT(VP_HANDLE) VpSetPosZero(VP_HANDLE a);
VP_EXPORT(VP_HANDLE) VpSetNegZero(VP_HANDLE a);
VP_EXPORT(VP_HANDLE) VpSetZero(VP_HANDLE a,int s);

/* NaN */
VP_EXPORT(int)       VpIsNaN(VP_HANDLE a);
VP_EXPORT(VP_HANDLE) VpSetNaN(VP_HANDLE a);

/* Infinity */
VP_EXPORT(int)       VpIsPosInf(VP_HANDLE a);
VP_EXPORT(int)       VpIsNegInf(VP_HANDLE a);
VP_EXPORT(int)       VpIsInf(VP_HANDLE a);
VP_EXPORT(VP_HANDLE) VpSetPosInf(VP_HANDLE a);
VP_EXPORT(VP_HANDLE) VpSetNegInf(VP_HANDLE a);
VP_EXPORT(VP_HANDLE) VpSetInf(VP_HANDLE a,int s);

VP_EXPORT(int)       VpGetRoundMode();
VP_EXPORT(int)       VpSetRoundMode(int m);
VP_EXPORT(VP_HANDLE) VpAdd(VP_HANDLE c,VP_HANDLE a,VP_HANDLE b);
VP_EXPORT(VP_HANDLE) VpSub(VP_HANDLE c,VP_HANDLE a,VP_HANDLE b);
VP_EXPORT(VP_HANDLE) VpMul(VP_HANDLE c,VP_HANDLE a,VP_HANDLE b);
VP_EXPORT(VP_HANDLE) VpDiv(VP_HANDLE c, VP_HANDLE r, VP_HANDLE a, VP_HANDLE b);
VP_EXPORT(int)       VpCmp(VP_HANDLE a, VP_HANDLE b);
VP_EXPORT(VP_HANDLE) VpAsgn(VP_HANDLE C, VP_HANDLE A, int isw);
VP_EXPORT(VP_HANDLE) VpAsgn2(VP_HANDLE C, VP_HANDLE A, int isw,int round);
VP_EXPORT(VP_HANDLE) VpScaleRound(VP_HANDLE p, int ixRound);
VP_EXPORT(VP_HANDLE) VpLengthRound(VP_HANDLE p, int ixRound);
VP_EXPORT(VP_HANDLE) VpScaleRound2(VP_HANDLE p, int ixRound,int mode);
VP_EXPORT(VP_HANDLE) VpLengthRound2(VP_HANDLE p, int ixRound,int mode);
VP_EXPORT(VP_HANDLE) VpFrac(VP_HANDLE y, VP_HANDLE x);
VP_EXPORT(VP_HANDLE) VpFix(VP_HANDLE y,VP_HANDLE x);
#define VpInt(x,y)   VpFix(x,y)

/* Math function */
VP_EXPORT(VP_UINT)   VpGetMaxIterationCount();
VP_EXPORT(VP_UINT)   VpSetMaxIterationCount(VP_UINT c);
VP_EXPORT(VP_UINT)   VpGetIterationCount();
VP_EXPORT(VP_HANDLE) VpSqrt(VP_HANDLE hy, VP_HANDLE hx);
VP_EXPORT(VP_HANDLE) VpPower(VP_HANDLE hy, VP_HANDLE hx, int n);
VP_EXPORT(VP_HANDLE) VpPI(VP_HANDLE h); /* h = PI */
VP_EXPORT(VP_HANDLE) VpExp(VP_HANDLE y,VP_HANDLE x);
VP_EXPORT(VP_HANDLE) VpSin(VP_HANDLE y,VP_HANDLE x);
VP_EXPORT(VP_HANDLE) VpCos(VP_HANDLE y,VP_HANDLE x);
VP_EXPORT(VP_HANDLE) VpAtan(VP_HANDLE y,VP_HANDLE x);
VP_EXPORT(VP_HANDLE) VpLog(VP_HANDLE y,VP_HANDLE x);

#if defined(__cplusplus)
}  /* extern "C" { */
#endif
#endif /* _BIG_DECIMAL_H */
