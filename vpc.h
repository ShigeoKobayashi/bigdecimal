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

 /* Extern items defined in vpc.c */
extern int      gcError;   /* flag for error(error counter in a line) */
extern int      gfQuit;    /* flag for 'quit' */
extern int      gfBreak;   /* flag for 'break' */

#define ERROR(s)  {gcError++; s ;}
#define FATAL(s)  {s ; FinishVpc(-1);}
#define IsError() (gcError>0)
#define IsQuit()  (gfQuit!=0)

/* TOKEN.what */
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
	int   start;  /* start position of input token buffer */
	int   end;
} STATEMENT;

typedef struct _FUNCTION {
	const UCHAR *name;
	void*       func;
	int         arguments;
} FUNCTION;

/* A line consists of statements, and a statement consists of tokens. */
typedef struct _READER {
	FILE*      file;         /* The file processing */
	char*      Buffer;       /* Character buffer for one line to be read */
	int        mBuffer;      /* Max size of 'Buffer' */
	int        cBuffer;      /* character size read to gInputBuffer */

	STATEMENT* Statements;   /* Token count read to gTokens of 'one' statement */
	int        mStatements;  /* Max statement count */
	int        cStatements;  /* statement counter used  */

	TOKEN*     Tokens;       /* Token buffer */
	int        mTokens;      /* Max tokens */
	int        cTokens;      /* token counter */

	int        iStatement;   /* Current statement processed */
} READER;

typedef struct _P_STATEMENT {
	int start;
	int end;
} P_STATEMENT;

typedef struct _PARSER {
	READER* r;
	int* TokenStack;      /* Temporaly stack for creating reverse polish */
	int    cTokenStack;

	int* TotalPolish;     /* Total buffer of reverse polish in a line */
	int    cTotalPolish;

	P_STATEMENT* PolishDivider; /* start end end points of reverse polish of each statement in a line */

	int* Polish;           /* temporary reverse polish array to execute */
	int   cPolish;

	int    mSize;            /* max size of arrays above(all the same) */

} PARSER;


extern READER* OpenReader(FILE *f, int max_line);
extern void    CloseReader(READER* r);
extern void    SetStatement(READER* r, int i);
extern int     GetStatement(READER* r);

extern UCHAR IsDEL(UCHAR ch);
extern int   IsEOL(UCHAR ch);
extern int   IsEOS(UCHAR ch);
extern int   IsEOF(UCHAR ch);
extern UCHAR ReadLine(READER* r);
extern int   IsDigit(UCHAR ch);

extern int    TokenCount(READER* r);
extern int    TokenSize(READER* r, int it);
extern UCHAR  TokenChar(READER* r, int it, int ic);
extern int    TokenWhat(READER* r, int it);
extern int    TokenWhat2(READER* r, int it);
extern int    TokenPriority(READER* r, int it);
extern void   SetTokenWhat2(READER* r, int it, int w);
extern UCHAR* TokenPTR(READER* r, int it);
extern void   SetTokenWhat(READER* r, int it, int w);
extern void   SetTokenPriority(READER* r, int it, int p);
extern int    IsNumeric(READER* r, int it);
extern int    IsToken(const UCHAR* token, READER* r, int it);

/* in parser.c */

extern void   InitVpc(int cInput, int cToken);
extern void   FinishVpc(int e);
extern void   ClearGlobal();

extern UCHAR  gDelimiters[];
extern int    gmDelimiters;

extern void   ReadAndExecuteLines(FILE* f);
extern void   ParseAndExecuteRepeat(PARSER *p);
extern void   ParseAndExecuteWhile(PARSER* p,int nt);
extern void   ParseAndExecuteIf(PARSER* p, int nt);
extern void ParseAndExecuteLoad(PARSER* p, int nt);

/* Extern items defined in parser.c */
extern int    ParseStatement(PARSER* p);
extern void   ExecuteStatement(PARSER *p,int iStatement);

typedef struct _SETTING {
	const UCHAR* name;
	int         value;
	void* print;
	void* calc;
} SETTING;


extern SETTING gSetting[];
extern int     gmSetting;


/* Extern items defined in calculator.c */
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

/* Extern items define in setting.c */
extern int   gmPrecision;
extern int   gnCount;
extern UCHAR gchFormatChar;
extern UCHAR gchSeparator;
extern UCHAR gchQuote;
extern UCHAR gchLeader;

/* print functions refferred in gSetting[].print  */
extern void PrintTitle        (PARSER* p, FILE* f, int newline);
extern void PrintFormat       (PARSER* p, FILE* f, int newline);
extern void PrintPrecision    (PARSER* p, FILE* f, int newline);
extern void PrintIterations   (PARSER* p, FILE* f, int newline);
extern void PrintRound        (PARSER* p, FILE* f, int newline);
extern void PrintVariableTitle(PARSER* p, FILE* f, int newline);

/* setting functions refferred in gSetting[].calc  */
extern void DoTitle(PARSER* p);
extern void DoFormat(PARSER* p);
extern void DoPrecision(PARSER* p);
extern void DoIterations(PARSER* p);
extern void DoRound(PARSER* p);
extern void SetVTitle(PARSER* p);



extern void OutputVariableTitle(FILE* f, UCHAR chv,int newline);
extern void PrintVariable(FILE* f, UCHAR chv, int newline);

extern void DoPrint(PARSER* p);
extern void DoSetting(PARSER* p);

/* Extern items define in io.c */
extern void DoRead  (UCHAR* inFile);
extern void DoWrite (PARSER *p,UCHAR* otFile);

/* --- end of vpc.h --- */
