/*
 *
 * BigDecimal(Variable decimal precision) extension library.
 *
 * Copyright(C) 2012 by Shigeo Kobayashi(shigeo@tinyforest.jp)
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
#define VP_DIGIT   unsigned long       /* Fraction part array    */
#define VP_UINT    size_t              /* Common unsigned part   */

/*
 * VP representation
 *  r = 0.xxxxxxxxx *BASE**exponent
 */
typedef struct {
	VP_UINT   Size;    /* all byte size of this structure(used in realloc() case).  */
    VP_UINT   MaxPrec; /* Maximum precision size                   */
                       /* This is the actual size of frac[]        */
                       /*(frac[0] to frac[MaxPrec] are available). */
    VP_UINT   Prec;    /* Current precision size.                  */
                       /* This indicates how much the.             */
                       /* the array frac[] is actually used.       */
	VP_UINT   Dummy;   /* Dummy for structure alignment (8byte)    */
    int       exponent;/* Exponent part.                           */
    int       sign;    /* Attributes of the value.                 */
                       /*
                        *        ==0 : NaN
                        *          1 : Positive zero
                        *         -1 : Negative zero
                        *          2 : Positive number
                        *         -2 : Negative number
                        *          3 : Positive infinite number
                        *         -3 : Negative infinite number
                        */
    VP_DIGIT   frac[1]; /* Array of fraction part. */
} Real;

#define VP_SIGN_NaN                0 /* NaN                      */
#define VP_SIGN_POSITIVE_ZERO      1 /* Positive zero            */
#define VP_SIGN_NEGATIVE_ZERO     -1 /* Negative zero            */
#define VP_SIGN_POSITIVE_FINITE    2 /* Positive finite number   */
#define VP_SIGN_NEGATIVE_FINITE   -2 /* Negative finite number   */
#define VP_SIGN_POSITIVE_INFINITE  3 /* Positive infinite number */
#define VP_SIGN_NEGATIVE_INFINITE -3 /* Negative infinite number */

#define VpBadHandle(h)  (((VP_HANDLE)h)<100)
#define VpIsHandle(h)   (((VP_HANDLE)h)>100)

/* ERROR CODES */
#define VP_ERROR_BAD_STRING         1
#define VP_ERROR_BAD_HANDLE         2
#define VP_ERROR_MEMORY_ALLOCATION  3


/*
 *  NaN & Infinity
 */
#define SZ_NaN  "NaN"
#define SZ_INF  "Infinity"
#define SZ_PINF "+Infinity"
#define SZ_NINF "-Infinity"

/* Exception codes */
#define VP_EXCEPTION_ALL        ((unsigned short)0x00FF)
#define VP_EXCEPTION_INFINITY   ((unsigned short)0x0001)
#define VP_EXCEPTION_NaN        ((unsigned short)0x0002)
#define VP_EXCEPTION_UNDERFLOW  ((unsigned short)0x0004)
#define VP_EXCEPTION_OVERFLOW   ((unsigned short)0x0001) /* 0x0008) */
#define VP_EXCEPTION_ZERODIVIDE ((unsigned short)0x0010)

/* Following 2 exceptions cann't controlled by user */
#define VP_EXCEPTION_OP         ((unsigned short)0x0020)
#define VP_EXCEPTION_MEMORY     ((unsigned short)0x0040)

/* Computation mode */
#define VP_ROUND_MODE            ((unsigned short)0x0100)

#define VP_ROUND_UP         1
#define VP_ROUND_DOWN       2
#define VP_ROUND_HALF_UP    3
#define VP_ROUND_HALF_DOWN  4
#define VP_ROUND_CEIL       5
#define VP_ROUND_FLOOR      6
#define VP_ROUND_HALF_EVEN  7


/*
  ERROR codes
*/
#define VP_ERROR_NOT_CONVERGED -9   // Iteration not converged,
#define VP_ERROR_BAD_LEFT      -99  // ���ӂ��s��.



VP_EXPORT(VP_HANDLE) VpAlloc(char *szVal,VP_UINT mx);
VP_EXPORT(int)       VpAllocCount(); /* returns VP_HANDLE allocation count */
VP_EXPORT(void)      VpFree(VP_HANDLE *p);
VP_EXPORT(int)       VpPrintE(FILE *fp, VP_HANDLE h);
VP_EXPORT(VP_UINT)   VpEffectiveDigits(VP_HANDLE h);
VP_EXPORT(VP_UINT)   VpStringLengthE(VP_HANDLE h);
VP_EXPORT(VP_UINT)   VpStringLengthF(VP_HANDLE h);
VP_EXPORT(char *)    VpToStringE(VP_HANDLE h,char *sz);
VP_EXPORT(char *)    VpToStringF(VP_HANDLE h,char *sz);
VP_EXPORT(int)  	 VpExponent(VP_HANDLE h);
VP_EXPORT(VP_HANDLE) VpAsgn(VP_HANDLE C, VP_HANDLE A, int isw);
VP_EXPORT(VP_HANDLE) VpMidRound(VP_HANDLE p, int f, int nf);
VP_EXPORT(VP_HANDLE) VpLengthRound(VP_HANDLE p, int f, int nf);



/* Sign */
/* Change sign of a to a>0,a<0 if s = 1,-1 respectively */
VP_EXPORT(VP_HANDLE) VpSetSign(VP_HANDLE a,int s);
VP_EXPORT(int)       VpGetSign(VP_HANDLE a);
VP_EXPORT(VP_HANDLE) VpRevertSign(VP_HANDLE a,int s);

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
VP_EXPORT(int)       VpIsDef(VP_HANDLE a);
VP_EXPORT(VP_HANDLE) VpSetPosInf(VP_HANDLE a);
VP_EXPORT(VP_HANDLE) VpSetNegInf(VP_HANDLE a);
VP_EXPORT(VP_HANDLE) VpSetInf(VP_HANDLE a,int s);
VP_EXPORT(int)       VpHasVal(VP_HANDLE a);
VP_EXPORT(int)       VpGetRoundMode();
VP_EXPORT(int)       VpSetRoundMode(int m);
VP_EXPORT(VP_UINT)   VpGetDigitSeparationCount();
VP_EXPORT(VP_UINT)   VpSetDigitSeparationCount(VP_UINT m);
VP_EXPORT(char)      VpGetDigitSeparator();
VP_EXPORT(char)      VpSetDigitSeparator(char c);
VP_EXPORT(char)      VpGetDigitsAlign();
VP_EXPORT(char)      VpSetDigitsAlign(char c);





#if defined(__cplusplus)
}  /* extern "C" { */
#endif
#endif /* _BIG_DECIMAL_H */