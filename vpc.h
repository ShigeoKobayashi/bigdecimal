/*
 * Variable precision calculator.
 * (Test program for Bigdecimal(Variable decimal precision) C/C++ library.)
 *
 *  Copyright(C) 2018 by Shigeo Kobayashi(shigeo@tinyforest.jp)
 *
 *    Version 2.0
 *
 */

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define strcasecmp(a,b) _strcmpi(a,b)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "bigdecimal.h"

#define UCHAR unsigned char

#define ERROR(s)   {gcError++; s ;}
#define FATAL(s)  {s ; FinishVpc(-1);}

/* NODE.what */
#define VPC_VARIABLE   1  /* a,b,c,..,z or R                         */
#define VPC_NUMERIC    2  /* 1,2,3,... or '123...'                   */
#define VPC_SETTING    3  /* $format,$precision etc                  */
#define VPC_PRINT      4  /* ?                                       */
#define VPC_BRA        5  /*  (                                      */
#define VPC_KET        6  /*  )                                      */
#define VPC_UOPERATOR  7  /*  unary  operator:  + -  (+ ignored)     */
#define VPC_BOPERATOR  8  /*  binary operator:  + - * / =            */
#define VPC_COMMA      9  /* .                                       */
#define VPC_FUNC      10  /* sin cos etc                             */
#define VPC_UNDEFINED -1  /* other than aboves.                      */

typedef struct _TOKEN {
	int   start;  /* start position of input character buffer */
	int   end;
	int   what;
	int   etc;
} TOKEN;

typedef struct _STATEMENT {
	int   start;  /* start position of input character buffer */
	int   end;
} STATEMENT;


typedef struct _FUNCTION {
	const UCHAR *name;
	void*       func;
	int         arguments;
} FUNCTION;

/* Extern items defined in vpc.c */
extern int      gcError;  /* flag for error(error counter in a line) */
extern int      gnRepeat; /* repeat counter */
extern int      gfWhile;  /* flag for 'while' */

extern void   InitVpc(int cInput, int cToken);
extern void   FinishVpc(int e);


/* Extern items defined in reader.c */
extern UCHAR*     gInputBuffer;   /* read buffer */
extern int        gmInputBuffer;  /* Max buffer character size*/
extern int        gcInputBuffer;  /* Character size read to gInputBuffer */

extern TOKEN*     gTokens;        /* gTokens[gmTokens] ,gTokens[i] corresponds to gTokens[i] */
extern TOKEN*     gTokens;        /* Token buffer */
extern int        gmTokens;       /* Max token count */
extern int        gcTokens;       /* token count */
extern STATEMENT* gStatements;    /* statements */
extern int        gmStatements;   /* max statement in a line */
extern int        gcStatements;   /* statement counter */

extern UCHAR  gDelimiters[];
extern int    gmDelimiters;

extern void   InitReader(int buffer_size, int token_size);
extern void   FinishReader();
extern void   ReadLines(FILE* f);
extern void   DoRepeat(UCHAR* repeat);
extern void   DoWhile(int iStatement,int nt);
extern int    IsToken(const UCHAR* token, int iStatement,int it);
extern int    TokenCount(int iStatement);
extern int    TokenSize(int iStatement,int it);
extern UCHAR  TokenChar(int iStatement,int it, int ic);
extern int    TokenWhat(int iStatement,int it);
extern int    TokenWhat2(int iStatement,int it);
extern int    TokenPriority(int iStatement,int it);
extern int    TokenFrom(int iStatement,int it);
extern int    TokenTo(int iStatement,int it);
extern void   SetTokenWhat2(int iStatement,int it, int w);
extern UCHAR* TokenPTR(int iStatement,int it);
extern void   SetTokenWhat(int iStatement,int it, int w);
extern void   SetTokenPriority(int iStatement,int it, int p);


/* Extern items defined in parser.c */
extern void   InitParser();
extern void   FinishParser();
extern int    ParseStatement(int iStatement);
extern int    ExecuteStatement(int iStatement);

typedef struct _SETTING {
	const UCHAR* name;
	int         value;
	void* print;
	void* calc;
} SETTING;

extern SETTING gSetting[];
extern int     gmSetting;


/* Extern items defined in calculator.c */
extern void         InitCalculator();
extern void         FinishCalculator();
extern void         ComputePolish();
extern int          ToIntFromSz(int* pi, UCHAR* sz);
extern int          CreateNumericWorkVariable(UCHAR* szN);

extern FUNCTION     gFunctions[]; /* sin,cos,... etc */
extern int          gmFunctions;  /* Max functions count */
extern VP_HANDLE    gVariables[];
extern int          gmVariables;
extern VP_HANDLE    gWorkVariables[];
extern int          gmWorkVariables;

extern int          EnsureVariable(int iv, int mx);
extern const UCHAR* FunctionName(int i);
extern int          FunctionArguments(int i);
extern int*         gPolish;
extern int          gcPolish;

/* Extern items define in setting.c */
extern int   gmPrecision;
extern int   gnCount;
extern UCHAR gchFormatChar;
extern UCHAR gchSeparator;
extern UCHAR gchQuote;
extern UCHAR gchLeader;

extern void SetVTitle(int iStatement);
extern void PrintVariableTitle(FILE* f, int iStatement);
extern void OutputVariableTitle(FILE* f, UCHAR chv);
extern void PrintFormat();
extern void PrintFormat(FILE* f);
extern void PrintPrecision(FILE* f);
extern void PrintRound(FILE* f);
extern void PrintIterations(FILE* f);
extern void PrintTitle(FILE* f);
extern void PrintVariable(FILE* f, UCHAR chv);
extern void DoPrint(int iStatement);
extern void DoSetting(int iStatement);
extern void DoTitle(int iStatement);
extern void DoFormat(int iStatement);
extern void DoPrecision(int iStatement);
extern void DoIterations(int iStatement);
extern void DoRound(int iStatement);

/* Extern items define in io.c */
extern void DoRead  (UCHAR* inFile);
extern void DoWrite (UCHAR* otFile);

/* --- end of vpc.h --- */
